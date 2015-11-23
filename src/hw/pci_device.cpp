#include <common>
#include <pci_device.hpp>
#include <syscalls.hpp>
//#include <class_nic.hpp>
#include <assert.h>


#define NUM_CLASSCODES 19
static const char* classcodes[NUM_CLASSCODES]={
  "Too-Old-To-Tell",           // 0
  "Mass Storage Controller",   // 1
  "Network Controller",        // 2
  "Display Controller",        // 3
  "Multimedia Controller",     // 4
  "Memory Controller",         // 5
  "Bridge",                    // 6
  "Simple communications controllers",
  "Base system peripherals",   // 8
  "Inupt device",              // 9
  "Docking Station",
  "Processor",
  "Serial Bus Controller",
  "Wireless Controller",
  "Intelligent I/O Controller",
  "Satellite Communication Controller", // 15
  "Encryption/Decryption Controller",   // 16
  "Data Acquisition and Signal Processing Controller",  // 17
  NULL
};

const int SS_BR=3;
static const char* bridge_subclasses[SS_BR]={
  "Host",
  "ISA",
  "Other"
};

const int SS_NIC=2;
static const char* nic_subclasses[SS_NIC]={
  "Ethernet",
  "Other"
};



struct _pci_vendor{
  uint16_t id;
  const char* name;
} _pci_vendorlist[]={
  {0x8086,"Intel Corp."},
  {0x1013,"Cirrus Logic"},
  {0x10EC,"Realtek Semi.Corp."},
  {0x1AF4,"Virtio (Rusty Russell)"}, //Virtio creator
  {0x1022,"AMD"},
  {0x0000,NULL}
};


static unsigned long pci_size(unsigned long base, unsigned long mask)
{
  // Find the significant bits
  unsigned long size = mask & base;

  // Get the lowest of them to find the decode size
  size = size & ~(size-1);

  return size;
}


uint32_t PCI_Device::iobase()
{
  assert(res_io_ != 0);
  return res_io_->start_;  
};

void PCI_Device::probe_resources(){

  //Find resources on this PCI device (scan the BAR's)
  uint32_t value=PCI::WTF;
  
  uint32_t reg{0},len{0};
  for(int bar=0; bar<6; bar++){

    //Read the current BAR register   
    reg = PCI::CONFIG_BASE_ADDR_0 + (bar << 2);
    value = read_dword(reg);

    if (!value) continue;

    //Write all 1's to the register, to get the length value (osdev)
    write_dword(reg,0xFFFFFFFF);
    len = read_dword(reg);
    
    //Put the value back
    write_dword(reg,value);
    
    uint32_t unmasked_val=0, pci_size_=0;

    if (value & 1) {  // Resource type IO

      unmasked_val = value & PCI::BASE_ADDRESS_IO_MASK;
      pci_size_ = pci_size(len,PCI::BASE_ADDRESS_IO_MASK & 0xFFFF );
      
      //Add it to resource list
      add_resource<RES_IO>(new Resource<RES_IO>(unmasked_val,pci_size_),res_io_);
      assert(res_io_ != 0);            
      
    } else { //Resource type Mem

      unmasked_val = value & PCI::BASE_ADDRESS_MEM_MASK;
      pci_size_ = pci_size(len,PCI::BASE_ADDRESS_MEM_MASK);

      //Add it to resource list
      add_resource<RES_MEM>(new Resource<RES_MEM>(unmasked_val,pci_size_),res_mem_);
      assert(res_mem_ != 0);
    }    
    INFO2("");
    INFO2("[ Resource @ BAR %i ]", bar);
    INFO2("  Address:  0x%x Size: 0x%x", unmasked_val, pci_size_);
    INFO2("  Type: %s", value & 1 ? "IO Resource" : "Memory Resource");    
  }
  
  INFO2("");
  
}

PCI_Device::PCI_Device(uint16_t pci_addr,uint32_t _id)
  : pci_addr_(pci_addr), device_id_{_id}
//,Device(Device::PCI) //Why not inherit Device? Well, I think "PCI devices" are too general to be useful by itself, and the "Device" class is Public ABI, so it should only know about stuff that's relevant for the user.
{
  //We have device, so probe for details
  devtype_.reg=read_dword(pci_addr,PCI::CONFIG_CLASS_REV);
  //printf("\t * New PCI Device: Vendor: 0x%x Prod: 0x%x Class: 0x%x\n", 
  //device_id.vendor,device_id.product,classcode);
  
  INFO2("|");  
  
  switch (devtype_.classcode) {
    
  case PCI::BRIDGE:
    INFO2("+--+ %s %s (0x%x)",
           bridge_subclasses[devtype_.subclass < SS_BR ? devtype_.subclass : SS_BR-1],
           classcodes[devtype_.classcode],devtype_.subclass);
    break;
  case PCI::NIC:
    INFO2("+--+ %s %s (0x%x)",
           nic_subclasses[devtype_.subclass < SS_NIC ? devtype_.subclass : SS_NIC-1],
           classcodes[devtype_.classcode],devtype_.subclass);             
    
    break;
  default:
    
    if (devtype_.classcode < NUM_CLASSCODES) {
      INFO2("+--+ %s ",classcodes[devtype_.classcode]);
    } else {
      INFO2("\t +--+ Other (Classcode 0x%x) \n",devtype_.classcode);
    }
      
  }

  
}


void PCI_Device::write_dword(uint8_t reg,uint32_t value){
  PCI::msg req;
  req.data=0x80000000;
  req.addr=pci_addr_;
  req.reg=reg;
  
  outpd(PCI::CONFIG_ADDR,(uint32_t)0x80000000 | req.data );
  outpd(PCI::CONFIG_DATA, value);
};

uint32_t PCI_Device::read_dword(uint8_t reg){
  PCI::msg req;
  req.data=0x80000000;
  req.addr=pci_addr_;
  req.reg=reg;
    
  outpd(PCI::CONFIG_ADDR,(uint32_t)0x80000000 | req.data );
  return inpd(PCI::CONFIG_DATA);
};

uint32_t PCI_Device::read_dword(uint16_t pci_addr, uint8_t reg){
  PCI::msg req;
    req.data=0x80000000;
    req.addr=pci_addr;
    req.reg=reg;
    
    outpd(PCI::CONFIG_ADDR,(uint32_t)0x80000000 | req.data );
    return inpd(PCI::CONFIG_DATA);
  };  




/** INLINED */

//uint16_t PCI_Device::pci_addr(){ return pci_addr_; }; //Inlined

//classcode_t PCI_Device::classcode(){ return static_cast<classcode_t>(devtype_.classcode); };

//uint16_t PCI_Device::vendor_id(){ return device_id_.vendor; }

  
  //TODO: Subclass this (or add it as member to a class) into device types.
  //...At least for NICs and HDDs
  

