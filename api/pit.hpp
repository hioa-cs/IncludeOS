#pragma once
#include <delegate>
#include <chrono>
#include <map>
#include <hertz>

/**
   Programmable Interval Timer class. A singleton.
   
   @TODO
   ...It has timer-functionality, which should probably be super-classed, 
   so that i.e. the HPET could be used with the same interface.
*/
class PIT{     
public:
  
  typedef delegate<void()> timeout_handler;
  typedef std::function<bool()> repeat_condition;
  
  /** Create a one-shot timer. 
      @param ms: Expiration time. Compatible with all std::chrono durations.
      @param handler: A delegate or function to be called on timeout.   */
  void onTimeout(std::chrono::milliseconds ms, timeout_handler handler);

  /** Create a repeating timer. 
      @param ms: Expiration time. Compatible with all std::chrono durations.
      @param handler: A delegate or function to be called every ms interval. 
      @param cond: The timer ends when cond() returns false. Default to true. */
  void onRepeatedTimeout(std::chrono::milliseconds ms, 
			 timeout_handler handler, 
			 repeat_condition cond = forever);
  
  /** No copy or move. The OS owns one instance forever. */
  PIT(PIT&) = delete;
  PIT(PIT&&) = delete;
    
  /** Get the (single) instance. */
  static PIT& instance() { 
    static PIT instance_;
    return instance_;
  };
  
  /** Initialize the hardware. */
  static void init();    
  
  /** The constant frequency of the PIT-hardware, before frequency dividers */
  static constexpr MHz frequency() { return frequency_; }
  
  static inline MHz current_frequency(){ return frequency() / current_freq_divider_; }
  
  /** Estimate cpu frequency based on the fixed PIT frequency and rdtsc. 
      @Note This is an asynchronous function. Once finished the result can be 
            fetched by CPUFrequency() (below)  */
  static void estimateCPUFrequency();
  
  /** Get the last estimated CPU frequency.  May trigger frequency sampling */
  static MHz CPUFrequency();
  
private: 
  // Default repeat-condition
  static std::function<bool()> forever;
  
  enum Mode { ONE_SHOT = 0, 
	      HW_ONESHOT = 1 << 1, 
	      RATE_GEN = 2 << 1, 
	      SQ_WAVE = 3 << 1, 
	      SW_STROBE = 4 << 1, 
	      HW_STROBE = 5 << 1, 
	      NONE = 256};
  
  // The PIT-chip runs at this fixed frequency (in MHz) , according to OSDev.org */
  static constexpr MHz frequency_ = MHz(14.31818 / 12);

  /** Disable regular timer interrupts- which are turned on at boot-time. */
  static void disable_regular_interrupts();
  
  /**  The default (soft)handler for timer interrupts */
  void irq_handler();
    
  // Private constructor / destructor. It's a singleton.
  PIT();  
  ~PIT();


  // State-keeping
  static Mode temp_mode_;
  static uint16_t temp_freq_divider_;  
  static uint8_t status_byte_;
  static uint16_t current_freq_divider_;
  static Mode current_mode_;
  static uint64_t IRQ_counter_;

  
  // The closest we can get to a millisecond interval, with the PIT-frequency
  static constexpr uint16_t  millisec_interval = KHz(frequency_).count();
  
  // Count the "milliseconds"
  static uint64_t millisec_counter;

  // Access mode bits are bits 4- and 5 in the Mode register
  enum AccessMode { LATCH_COUNT = 0x0, LO_ONLY=0x10, HI_ONLY=0x20, LO_HI=0x30 };
  
  /** Physically set the PIT-mode */
  static void set_mode(Mode);
  
  /** Physiclally set the PIT frequency divider */
  static void set_freq_divider(uint16_t);
  
  /** Set mode to one-shot, and frequency-divider to t */
  static void oneshot(uint16_t t);  
  
  /** Read back the PIT status from hardware */
  static uint8_t read_back(uint8_t channel);

  
  /** A timer is a handler and an expiration time (interval). 
      @todo The timer also keeps a pre-computed rdtsc-value, which is currently unused.*/
  class Timer {
  public:
    enum Type { ONE_SHOT, REPEAT, REPEAT_WHILE} type_;    
    
    Timer() = delete;    
    Timer(Type, timeout_handler, std::chrono::milliseconds, repeat_condition = forever);    
    Timer(const Timer&) = default;
    Timer(Timer&&) = default;
    Timer& operator=(Timer&) = default;
    Timer& operator=(Timer&&) = default;
    virtual ~Timer() = default;
    
    inline Type type(){ return type_; }
    inline std::chrono::milliseconds interval(){ return interval_; }
    inline uint64_t start() { return timestamp_start_; }
    inline uint64_t end() { return timestamp_end_; }
    inline void setStart(uint64_t s) { timestamp_start_ = s; }
    inline void setEnd(uint64_t e) { timestamp_end_ = e; } 
    inline timeout_handler handler(){ return handler_; }
    inline const repeat_condition cond() { return cond_; }
    inline uint32_t id(){ return id_; }
  
  private:
    static uint32_t timers_count_;
    uint32_t id_ = 0;
    timeout_handler handler_;
    uint64_t timestamp_start_;
    uint64_t timestamp_end_;
    std::chrono::milliseconds interval_;
    
    /* This Could be a reference in the default case of "forever", but then the
       case of a normal lambda being passed in, the user would have to be in charge
       of storage. */
    const repeat_condition cond_;
  };
  
  /** A map of timers. 
      @note {Performance: We take advantage of the fact that std::map have sorted keys. 
      * Timers soonest to expire are in the front, so we only iterate over those
      * Deletion of finished timers in amortized constant time, via iterators
      * Timer insertion is log(n) }
      @note This is why we want to instantiate PIT, and why it's a singleton: 
      If you don't use PIT-timers, you won't pay for them. */
  std::multimap<uint64_t,Timer> timers_;

  /** Queue the timer. This will update timestamps in the timer */
  void start_timer(Timer t, std::chrono::milliseconds);
  
};
