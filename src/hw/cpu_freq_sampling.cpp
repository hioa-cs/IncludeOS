//#define DEBUG
#include <hw/cpu_freq_sampling.hpp>
#include <common>
#include <vector>
#include <algorithm>
#include <os>
#include <irq_manager.hpp>

/** @note C-style code here, since we're dealing with interrupt handling. 
    The hardware expects a pure function pointer, and asm can't (easily) 
    call class member functions.  */

// This is how you provide storage for a static constexpr variable.
constexpr MHz PIT::frequency_;

extern "C" double _CPUFreq_ = 0;
extern "C" constexpr uint16_t _cpu_sampling_freq_divider_  = KHz(PIT::frequency()).count() * 10; // Run 1 KHz  Lowest: 0xffff

static constexpr int do_samples_ = 20;

std::vector<uint64_t> _cpu_timestamps;
std::vector<double> _cpu_freq_samples;

constexpr MHz test_frequency(){
  return MHz(PIT::frequency().count() / _cpu_sampling_freq_divider_);
}

MHz calculate_cpu_frequency(){
  
  // We expect the cpu_sampling_irq_handler to push in samples;
  while (_cpu_timestamps.size() < do_samples_)
    OS::halt();
      
  debug("_cpu_sampling_freq_divider_ : %i \n",_cpu_sampling_freq_divider_);
  
  #ifdef DEBUG
  for (auto t : _cpu_timestamps) debug("%lu \n",(uint32_t)t);
  #endif
  
  // Subtract the time it takes to measure time :-)
  auto t1 = OS::cycles_since_boot();
  OS::cycles_since_boot();
  auto t3 = OS::cycles_since_boot();
  auto overhead = (t3 - t1) * 2;
  
  debug ("Overhead: %lu \n", (uint32_t)overhead);
  
  for (size_t i = 1; i < _cpu_timestamps.size(); i++){
    // Compute delta in cycles
    auto cycles = _cpu_timestamps[i] - _cpu_timestamps[i-1] + overhead;
    // Cycles pr. second == Hertz
    auto freq = cycles / (1 / test_frequency().count());
    _cpu_freq_samples.push_back(freq);    
    debug("%lu - %lu = Delta: %lu Current PIT-Freq: %f Hz CPU Freq: %f MHz \n",
	  (uint32_t)_cpu_timestamps[i], (uint32_t)_cpu_timestamps[i-1],
	  (uint32_t)cycles, Hz(test_frequency()), freq);        
  }
  
  
#ifdef DEBUG
  double sum = 0;  
  for (auto freq : _cpu_freq_samples)
    sum += freq;  
  double mean = sum / _cpu_freq_samples.size();
#endif
  
  std::sort(_cpu_freq_samples.begin(), _cpu_freq_samples.end());
  double median = _cpu_freq_samples[_cpu_freq_samples.size() / 2];
  
  debug("<cpu_freq> MEAN: %f MEDIAN: %f \n",mean, median);
  _CPUFreq_ = median;
  
  return MHz(median);
  
}

void cpu_sampling_irq_handler(){
  
  auto t2 = OS::cycles_since_boot();
  
  if (_cpu_timestamps.size() < do_samples_)
    _cpu_timestamps.push_back(t2);
  
  IRQ_manager::eoi(0);
  return;
}  
