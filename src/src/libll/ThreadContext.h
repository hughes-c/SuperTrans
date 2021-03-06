#ifndef THREADCONTEXT_H
#define THREADCONTEXT_H

#include <stdint.h>
#include <vector>
#include <set>
#if (defined TM)
#include "transContext.h"
#endif
#include "Snippets.h"
#include "event.h"
#if (defined MIPS_EMUL)
#include "AddressSpace.h"
#include "SignalHandling.h"
#include "FileSys.h"
#include "InstDesc.h"


// Use this define to debug the simulated application
// It enables call stack tracking
//#define DEBUG_BENCH

#else
#include "HeapManager.h"
#include "icode.h"

#if defined(STAT_COMMON)
#include "stat-types.h"
#endif

#if (defined TLS)
namespace tls {
  class Epoch;
}
#endif

/* For future support of the exec() call. Information needed about
 * the object file goes here.
 */
typedef struct objfile {
  int rdata_size;
  int db_size;
  char *objname;
} objfile_t, *objfile_ptr;

/* The maximum number of file descriptors per thread. */
#define MAX_FDNUM 20

// Simulated machine has 32-bit registers
typedef int32_t IntRegValue;

enum IntRegName{
  RetValReg   =  2,
  RetValHiReg =  2,
  RetValLoReg =  3,
  IntArg1Reg  =  4,
  IntArg2Reg  =  5,
  IntArg3Reg  =  6,
  IntArg4Reg  =  7,
  JmpPtrReg   = 25,
  GlobPtrReg  = 28,
  StkPtrReg   = 29,
  RetAddrReg  = 31
};
#endif // Else of (defined MIPS_EMUL)

enum CpuMode{
  NoCpuMode,
  Mips32,
  Mips64,
};

#if (defined MIPS_EMUL)
class ThreadContext : public GCObject{
 public:
  typedef SmartPtr<ThreadContext> pointer;
 private:
  typedef std::vector<pointer> ContextVector;
#else
class ThreadContext {
  typedef std::vector<ThreadContext *> ContextVector;
#endif
  // Static variables
  static ContextVector pid2context;
  
#if !(defined MIPS_EMUL)
  static Pid_t baseClonedPid;
  static Pid_t nextClonedPid;
  static ContextVector clonedPool;

  static size_t nThreads;

  static Pid_t baseActualPid;
  static Pid_t nextActualPid;
  static ContextVector actualPool;
  static ThreadContext *mainThreadContext;
#endif //!(defined MIPS_EMUL)

  // Memory Mapping

#if !(defined MIPS_EMUL)
  // Lower and upper bound for valid data addresses
  VAddr dataVAddrLb;
  VAddr dataVAddrUb;
  // Lower and upper bound for stack addresses in all threads
  VAddr allStacksAddrLb;
  VAddr allStacksAddrUb;
#endif //!(defined MIPS_EMUL)
  // Lower and upper bound for stack addresses in this thread
  VAddr myStackAddrLb;
  VAddr myStackAddrUb;

#if !(defined MIPS_EMUL)
  // Real address is simply virtual address plus this offset
  RAddr virtToRealOffset;
#endif //!(defined MIPS_EMUL)

  // Local Variables
#if !(defined MIPS_EMUL)
 public:
  IntRegValue reg[33];	// The extra register is for writes to r0
  int lo;
  int hi;
  int abortCount;
  FILE *debug;
  FILE *trace;
#endif

#if !(defined MIPS_EMUL)
  unsigned int fcr0;	// floating point control register 0
  float fp[32];	        // floating point (and double) registers
  unsigned int fcr31;	// floating point control register 31
  icode_ptr picode;	// pointer to the next instruction (the "pc")
#endif
  int pid;		// process id
#if (defined TM)
  int tmDepth;          // Transactional Depth

  ID(
  int tmDebug;          // Transactional Debugging Mode Flag
  int tmDebugTrace;     // Transactional Debugging Trace Flag
    )
  int tmTrace;          // Transactional Trace Flag
  int tmTid;
  int tmAbortMax;       // Maximum Number of Aborts Allowed (after which aborts are just ignored)
  int tmAborting;       // Flag to Indicate Abort Initiated
  int tmNacking;        // Flag to Indicate NACK state
  transactionContext *transContext; // Transactional Context

  /*
    This flag will indicate whether the current begin in the fetch area is:
      0: First instantiation
      1: A Replay of an aborted instantiation
      2: A Subsumed Begin
  */
  int tmBCFlag;  
#endif

#if defined(STAT_COMMON)
private:
   THREAD_ID threadID;
public:
   THREAD_ID get_threadID() { return this->threadID; }
   BOOL set_threadID(THREAD_ID threadID) { this->threadID = threadID; return 1; }
#endif

    // Instruction pointer
  VAddr     iAddr;

   private:

#if !(defined MIPS_EMUL)
  RAddr raddr;		// real address computed by an event function
  icode_ptr target;	// place to store branch target during delay slot
#endif

#if (defined MIPS_EMUL)
  // Is the processor in 32bit or 64bit mode
  CpuMode cpuMode;
  // Register file(s)
  RegVal regs[NumOfRegs];
  // Address space for this thread
  AddressSpace::pointer addressSpace;
  // Instruction descriptor
  InstDesc *iDesc;
  // Virtual address generated by the last memory access instruction
  VAddr     dAddr;
#else
  HeapManager  *heapManager;  // Heap manager for this thread
  ThreadContext *parent;    // pointer to parent
  ThreadContext *youngest;  // pointer to youngest child
  ThreadContext *sibling;   // pointer to next older sibling
  int *perrno;	            // pointer to the errno variable
  int rerrno;		    // most recent errno for this thread

  char      *fd;	    // file descriptors; =1 means open, =0 means closed
#endif // Else branch of (defined MIPS_EMUL)

private:

#ifdef TASKSCALAR
  void badSpecThread(VAddr addr, short opflags) const;
  bool checkSpecThread(VAddr addr, short opflags) const {
    if (isValidDataVAddr(addr))
      return false;

#if 0
    bool badAlign = (MemBufferEntry::calcAccessMask(opflags
						    ,MemBufferEntry::calcChunkOffset(addr))
		     ) == 0;

    if (!badAddr && !badAlign)
      return false;
#endif
    badSpecThread(addr, opflags);
    return true;
  }
#endif

#if (defined TLS)
  tls::Epoch *myEpoch;
#endif

public:
#if (defined MIPS_EMUL)
  static inline int getPidUb(void){
    return pid2context.size();
  }
  inline void setMode(CpuMode newMode){
    cpuMode=newMode;
  }
  inline CpuMode getMode(void) const{
    return cpuMode;
  }

  inline const void *getReg(RegName name) const{
    return &(regs[name]);
  }
  inline void *getReg(RegName name){
    return &(regs[name]);
  }
  void clearRegs(void){
    memset(regs,0,sizeof(regs));
  }
  void save(ChkWriter &out) const;
  
#else
  inline IntRegValue getIntReg(IntRegName name) const {
    return reg[name];
  }
  inline void setIntReg(IntRegName name, IntRegValue value) {
    reg[name]=value;
  }
  
  inline IntRegValue getIntArg1(void) const{
    return getIntReg(IntArg1Reg);
  }
  inline IntRegValue getIntArg2(void) const{
    return getIntReg(IntArg2Reg);
  }
  inline IntRegValue getIntArg3(void) const{
    return getIntReg(IntArg3Reg);
  }
  inline IntRegValue getIntArg4(void) const{
    return getIntReg(IntArg4Reg);
  }
  inline VAddr getGlobPtr(void) const{
    return getIntReg(GlobPtrReg);
  }
  inline void setGlobPtr(VAddr addr){
    setIntReg(GlobPtrReg,addr);
  }
  inline VAddr getStkPtr(void) const{
    return getIntReg(StkPtrReg);
  }
  inline void setStkPtr(int val){
    I(sizeof(val)==4);
    setIntReg(StkPtrReg,val);
  }
  inline void setRetVal(int val){
    I(sizeof(val)==4);
    setIntReg(RetValReg,val);
  }
  inline void setRetVal64(long long int val){
    I(sizeof(val)==8);
    unsigned long long valLo=val;
    valLo&=0xFFFFFFFFllu;
    unsigned long long valHi=val;
    valHi>>=32;
    valHi&=0xFFFFFFFFllu;
    setIntReg(RetValLoReg,(IntRegValue)valLo);
    setIntReg(RetValHiReg,(IntRegValue)valHi);
  }
  

  inline icode_ptr getPCIcode(void) const{
    I((pid!=-1)||(picode==&invalidIcode));
    return picode;
  }
  inline void setPCIcode(icode_ptr nextIcode){
    I((pid!=-1)||(nextIcode==&invalidIcode));
    picode=nextIcode;
  }
  
  inline icode_ptr getRetIcode(void) const{
    return addr2icode(getIntReg(RetAddrReg));
  }
#endif // End else branch of (defined MIPS_EMUL)
  
  // Returns the pid of the thread (what would be returned by a getpid call)
  // In TLS, many contexts can share the same actual thread pid
  Pid_t getThreadPid(void) const;
  // Returns the pid of the context
  Pid_t getPid(void) const { return pid; }
  // Sets the pid of the context
  void setPid(Pid_t newPid){
    pid=newPid;
  }

#if !(defined MIPS_EMUL)

  int getErrno(void){
    return *perrno;
  }

  void setErrno(int newErrno){
    I(perrno);
    *perrno=newErrno;
  }
#endif

#if (defined TLS)
 void setEpoch(tls::Epoch *epoch){
   myEpoch=epoch;
 }
 tls::Epoch *getEpoch(void) const{
   return myEpoch;
 }
#endif // (defined TLS)

#if !(defined MIPS_EMUL)
  bool isCloned(void) const{ return (pid>=baseClonedPid); }
#endif // !(defined MIPS_EMUL)

  void copy(const ThreadContext *src);

#if !(defined MIPS_EMUL)
  unsigned int getFPUControl31() const { return fcr31; }
  void setFPUControl31(unsigned int v) {
    fcr31 = v;
  }

  unsigned int getFPUControl0() const { return fcr0; }
  void setFPUControl0(unsigned int v) {
    fcr0 = v;
  }

  int getREG(icode_ptr pi, int R) { return reg[pi->args[R]];}
  void setREG(icode_ptr pi, int R, int val) { 
    reg[pi->args[R]] = val;

  }

  void setREGFromMem(icode_ptr pi, int R, int *addr) {
#ifdef LENDIAN
    int val = SWAP_WORD(*addr);
#else
    int val = *addr;
#endif

    setREG(pi, R, val);
  }

  float getFP( icode_ptr pi, int R) { return fp[pi->args[R]]; }
  void  setFP( icode_ptr pi, int R, float val) { 
    fp[pi->args[R]] = val; 
  }
  void  setFPFromMem( icode_ptr pi, int R, float *addr) { 
    float *pos = &fp[pi->args[R]];
#ifdef LENDIAN
    unsigned int v1;
    v1 = *(unsigned int *)addr;
    v1 = SWAP_WORD(v1);
    *pos = *(float *)&v1;
#else
    *pos = *addr;
#endif
  }

  double getDP( icode_ptr pi, int R) const { 
#ifdef SPARC 
  // MIPS supports 32 bit align double access
    unsigned int w1 = *(unsigned int *) &fp[pi->args[R]];
    unsigned int w2 = *(unsigned int *) &fp[pi->args[R]+1];
    static unsigned long long ret = w2;
    ret = w2;
    ret = (ret<<32) | w1;
    return *(double *) (&ret);
#else 
    return *(double *) &fp[pi->args[R]];
#endif
  }

  void   setDP( icode_ptr pi, int R, double val) { 
#ifdef SPARC 
    unsigned int *pos = (unsigned int*)&fp[pi->args[R]];
    unsigned int b1 = ((unsigned int *)&val)[0];
    unsigned int b2 = ((unsigned int *)&val)[1];
    pos[0] = b1;
    pos[1] = b2;	
#else
    *((double *)&fp[pi->args[R]]) = val; 
#endif
  }


  void   setDPFromMem( icode_ptr pi, int R, double *addr) { 
#ifdef SPARC 
    unsigned int *pos = (unsigned int*) ((long)(fp) + pi->args[R]);
    pos[0] = (unsigned int) addr[0];
    pos[1] = (unsigned int) addr[1];
#else
    double *pos = (double *) &fp[pi->args[R]];
#ifdef LENDIAN
    unsigned long long v1;
    v1 = *(unsigned long long *)(addr);
    v1 = SWAP_LONG(v1);
    *pos = *(double *)&v1;

//     unsigned char* aRawBinary = reinterpret_cast<unsigned char*>((double *)&v1);
//     unsigned char* tRawBinary = reinterpret_cast<unsigned char*>(pos);
//     
// fprintf(stderr,"!!DOUBLEFP LDFP 0x%02x%02x%02x%02x%02x%02x%02x%02x\n",aRawBinary[0], aRawBinary[1], aRawBinary[2], aRawBinary[3],aRawBinary[4],aRawBinary[5],aRawBinary[6],aRawBinary[7]);
// 
// fprintf(stderr,"!!DOUBLEFP LDFP 0x%02x%02x%02x%02x%02x%02x%02x%02x\n",tRawBinary[0], tRawBinary[1], tRawBinary[2], tRawBinary[3],tRawBinary[4],tRawBinary[5],tRawBinary[6],tRawBinary[7]);
// 
// fprintf(stderr,"!! LDFP IS %f - %f\n",&pos,pos);
    
#else
    *pos = *addr;
#endif // LENDIAN
#endif // SPARC
  }

  int getWFP(icode_ptr pi, int R) { return *(int   *)&fp[pi->args[R]]; }
  void setWFP(icode_ptr pi, int R, int val) { 
    *((int   *)&fp[pi->args[R]]) = val; 
  }

  // Methods used by ops.m4 and coproc.m4 (mostly)
  int getREGNUM(int R) const { return reg[R]; }
  void setREGNUM(int R, int val) {
    reg[R] = val;
  }

  // FIXME: SPARC
  double getDPNUM(int R) {return *((double *)&fp[R]); }
  void   setDPNUM(int R, double val) {
    *((double *) &fp[R]) = val; 
  }

  // for constant (or unshifted) register indices
  float getFPNUM(int i) const { return fp[i]; }
  int getWFPNUM(int i) const  { return *((int *)&fp[i]); }

  RAddr getRAddr() const{
    return raddr;
  }
  void setRAddr(RAddr a){
    raddr = a;
  }

#if (defined TM)

  // Transactional Helper Methods
  
  int getTMdepth() const{
    return tmDepth;
  }

  void incTMdepth(){
    tmDepth++;
  }

  void decTMdepth(){
    tmDepth--;
  }
  
#endif

  
  void dump();
  void dumpStack();

  static void staticConstructor(void);

  static size_t size();
#endif // !(defined MIPS_EMUL)

  static ThreadContext *getContext(Pid_t pid);

#if (defined MIPS_EMUL)
  static ThreadContext *getMainThreadContext(void){
    return &(*(pid2context[0]));
  }
#else
  static ThreadContext *getMainThreadContext(void);

  static ThreadContext *newActual(void);
  static ThreadContext *newActual(Pid_t pid);
  static ThreadContext *newCloned(void);
  void free(void);
#endif // else of (defined MIPS_EMUL)


  static unsigned long long getMemValue(RAddr p, unsigned dsize); 

  // BEGIN Memory Mapping
  bool isValidDataVAddr(VAddr vaddr) const{
#if !(defined MIPS_EMUL)
    return (vaddr>=dataVAddrLb)&&(vaddr<dataVAddrUb);
#else
    return (addressSpace->virtToReal(vaddr)!=0);
#endif
  }

#if (defined MIPS_EMUL)
  ThreadContext(void);
  ThreadContext(ThreadContext &parent, bool shareAddrSpace, bool shareSigTable, bool shareOpenFiles, SignalID sig);
  ThreadContext(ChkReader &in);
  ~ThreadContext();

  ThreadContext *createChild(bool shareAddrSpace, bool shareSigTable, bool shareOpenFiles, SignalID sig);
  void setAddressSpace(AddressSpace *newAddressSpace){
    addressSpace=newAddressSpace;
  }
  AddressSpace *getAddressSpace(void) const{
    I(addressSpace);
    return addressSpace;
  }
#else // (defined MIPS_EMUL)
  void setHeapManager(HeapManager *newHeapManager){
    I(!heapManager);
    heapManager=newHeapManager;
    heapManager->addReference();
  }
  HeapManager *getHeapManager(void) const{
    I(heapManager);
    return heapManager;
  }
#endif // End else part of (defined MIPS_EMUL)
  inline void setStack(VAddr stackLb, VAddr stackUb){
    myStackAddrLb=stackLb;
    myStackAddrUb=stackUb;
  }
  inline VAddr getStackAddr(void) const{
    return myStackAddrLb;
  }
  inline VAddr getStackSize(void) const{
    return myStackAddrUb-myStackAddrLb;
  }
#if !(defined MIPS_EMUL)
  void initAddressing(VAddr dataVAddrLb, VAddr dataVAddrUb,
		      MINTAddrType rMap, MINTAddrType mMap, MINTAddrType sTop);
#endif // !(defined MIPS_EMUL)

  RAddr virt2real(VAddr vaddr, short opflags=E_READ | E_BYTE) const{
#ifdef TASKSCALAR
    if(checkSpecThread(vaddr, opflags))
      return 0;
#endif
#if (defined TLS)
    if(!isValidDataVAddr(vaddr))
      return 0;    
#endif
#if !(defined MIPS_EMUL)
#if !(defined TM)
    I(isValidDataVAddr(vaddr));
#endif
    return virtToRealOffset+vaddr;
#else // (defined MIPS_EMUL)
    return addressSpace->virtToReal(vaddr);
#endif // (defined MIPS_EMUL)
  }
  VAddr real2virt(RAddr raddr) const{
#if !(defined MIPS_EMUL)
    VAddr vaddr=raddr-virtToRealOffset;
    I(isValidDataVAddr(vaddr));
    return vaddr;
#else // For (defined MIPS_EMUL)
    return 0;
#endif // For else of (defined MIPS_EMUL)
  }
#if (defined MIPS_EMUL)
  inline InstDesc *virt2inst(VAddr vaddr){
    InstDesc *inst=addressSpace->virtToInst(vaddr);
    if(!inst){
      addressSpace->createTrace(this,vaddr);
      inst=addressSpace->virtToInst(vaddr);
    }
    return inst;
  }
#else // For (defined MIPS_EMUL)
  bool isHeapData(VAddr addr) const{
    I(heapManager);
    return heapManager->isHeapAddr(addr);
  }
#endif // For else of (defined MIPS_EMUL)

  bool isLocalStackData(VAddr addr) const {
    return (addr>=myStackAddrLb)&&(addr<myStackAddrUb);
  }

  VAddr getStackTop() const{
    return myStackAddrLb;
  }
  // END Memory Mapping

#if !(defined MIPS_EMUL)
  ThreadContext *getParent() const { return parent; }

  void useSameAddrSpace(ThreadContext *parent);
  void shareAddrSpace(ThreadContext *parent, int share_all, int copy_stack);
  void newChild(ThreadContext *child);
  void init();
#endif

#if (defined MIPS_EMUL)
  inline InstDesc *getIDesc(void) const {
    return iDesc;
  }
  inline void updIDesc(ssize_t ddiff){
    I((ddiff>=-1)&&(ddiff<4));
    iDesc+=ddiff;
  }
  inline VAddr getIAddr(void) const {
    return iAddr;
  }
  inline void setIAddr(VAddr addr){
    iAddr=addr;
    iDesc=iAddr?virt2inst(addr):0;
  }
  inline void updIAddr(ssize_t adiff, ssize_t ddiff){
    I((ddiff>=-1)&&(ddiff<4));
    I((adiff>=-4)&&(adiff<=8));
    iAddr+=adiff;
    iDesc+=ddiff;
  }
  inline VAddr getDAddr(void) const {
    return dAddr;
  }
  inline void setDAddr(VAddr addr){
    dAddr=addr;
  }
  static inline int nextReady(int startPid){
    int foundPid=startPid;
    do{
      if(foundPid==(int)(pid2context.size()))
        foundPid=0;
      ThreadContext *context=pid2context[foundPid];
      if(context&&(!context->isWaiting())&&(!context->isExited()))
        return foundPid;
      foundPid++;
    }while(foundPid!=startPid);
    return -1;      
  }
  static inline long long int skipInsts(long long int skipCount){
    long long int skipped=0;
    int nowPid=0;
    while(skipped<skipCount){
      nowPid=nextReady(nowPid);
      if(nowPid==-1)
        return skipped;
      ThreadContext *context=pid2context[nowPid];
      I(context);
      I(!context->isWaiting());
      I(!context->isExited());
      int nowSkip=(skipCount-skipped<100)?(skipCount-skipped):100;
      while(nowSkip&&context->skipInst()){
        nowSkip--;
        skipped++;
      }
      nowPid++;
    }
    return skipped;
  }
  inline bool skipInst(void){
    if(isWaiting())
      return false;
    if(isExited())
      return false;
#if (defined DEBUG_InstDesc)
    iDesc->debug();
#endif
    (*iDesc)(this);
    return true;
  }
#if (defined HAS_MEM_STATE)
  inline const MemState &getState(VAddr addr) const{
    return addressSpace->getState(addr);
  }
  inline MemState &getState(VAddr addr){
    return addressSpace->getState(addr);
  }
#endif
  inline bool canRead(VAddr addr, size_t len) const{
    return addressSpace->canRead(addr,len);
  }
  inline bool canWrite(VAddr addr, size_t len) const{
    return addressSpace->canWrite(addr,len);
  }
  void    writeMemFromBuf(VAddr addr, size_t len, const void *buf);
  ssize_t writeMemFromFile(VAddr addr, size_t len, int fd, bool natFile);
  void    writeMemWithByte(VAddr addr, size_t len, uint8_t c);  
  void    readMemToBuf(VAddr addr, size_t len, void *buf);
  ssize_t readMemToFile(VAddr addr, size_t len, int fd, bool natFile);
  ssize_t readMemString(VAddr stringVAddr, size_t maxSize, char *dstStr);
  template<class T>
  inline T readMemRaw(VAddr addr){
    if(sizeof(T)>sizeof(MemAlignType)){
      T tmp;
      I(canRead(addr,sizeof(T)));
      readMemToBuf(addr,sizeof(T),&tmp);
      return tmp;
    }
//    for(size_t i=0;i<(sizeof(T)+MemState::Granularity-1)/MemState::Granularity;i++)
//      if(getState(addr+i*MemState::Granularity).st==0)
//        fail("Uninitialized read found\n");
    return addressSpace->readMemRaw<T>(addr);
  }
  template<class T>
  inline bool writeMemRaw(VAddr addr, const T &val){
    if(sizeof(T)>sizeof(MemAlignType)){
      if(!canWrite(addr,sizeof(val)))
	return false;
      writeMemFromBuf(addr,sizeof(val),&val);
      return true;
    }
//    for(size_t i=0;i<(sizeof(T)+MemState::Granularity-1)/MemState::Granularity;i++)
//      getState(addr+i*MemState::Granularity).st=1;
    return addressSpace->writeMemRaw<T>(addr,val);
  }
  template<class T>
  inline T readMem(VAddr addr){
    I(canRead(addr,sizeof(T)));
    T tmp=readMemRaw<T>(addr);
    cvtEndianBig(tmp);
    return tmp;
  }
  template<class T>
  inline bool writeMem(VAddr addr, const T &val){
    T tmp=val;
    cvtEndianBig(tmp);    
    return writeMemRaw<T>(addr,tmp);
  }

  //
  // File system
  //
 private:
  FileSys::OpenFiles::pointer openFiles;
 public:
  void setOpenFiles(FileSys::OpenFiles *newOpenFiles){
    openFiles=newOpenFiles;
  }
  FileSys::OpenFiles *getOpenFiles(void) const{
    return openFiles;
  }

  //
  // Signal handling
  //
 private:
  SignalTable::pointer sigTable;
  SignalSet   sigMask;
  SignalQueue maskedSig;
  SignalQueue readySig;
  bool        suspSig;
 public:
  void setSignalTable(SignalTable *newSigTable){
    sigTable=newSigTable;
  }
  SignalTable *getSignalTable(void) const{
    return sigTable;
  }
  void suspend(void);
  void signal(SigInfo *sigInfo);
  void resume(void);
  const SignalSet &getSignalMask(void) const{
    return sigMask;
  }
  void setSignalMask(const SignalSet &newMask){
    sigMask=newMask;
    for(size_t i=0;i<maskedSig.size();i++){
      SignalID sig=maskedSig[i]->signo;
      if(!sigMask.test(sig)){
	readySig.push_back(maskedSig[i]);
	maskedSig[i]=maskedSig.back();
	maskedSig.pop_back();
      }
    }
    for(size_t i=0;i<readySig.size();i++){
      SignalID sig=readySig[i]->signo;
      if(sigMask.test(sig)){
	maskedSig.push_back(readySig[i]);
	readySig[i]=readySig.back();
	readySig.pop_back();
      }
    }
    if((!readySig.empty())&&suspSig)
      resume();
  }
  bool hasReadySignal(void) const{
    return !readySig.empty();
  }
  SigInfo *nextReadySignal(void){
    I(hasReadySignal());
    SigInfo *sigInfo=readySig.back();
    readySig.pop_back();
    return sigInfo;
  }

  // Process/Thread state

  // Parent/Child relationships
 private:
  int    parentID;
  typedef std::set<int> IntSet;
  IntSet childIDs;
  // Signal sent to parent when this thread dies/exits
  SignalID  exitSig;
 public:
  int  getParentID(void) const{
    return parentID;
  }
  bool hasChildren(void) const{
    return !childIDs.empty();
  }
  bool isChildID(int id) const{
    return (childIDs.find(id)!=childIDs.end());
  }
  int findZombieChild(void) const;
  SignalID getExitSig(void){
    return exitSig;
  }
 private:
  bool     exited;
  int      exitCode;
  SignalID killSignal;
 public:
  bool isWaiting(void) const{
    return suspSig;
  }
  bool isExited(void) const{
    return exited;
  }
  int getExitCode(void) const{
    return exitCode;
  }
  bool isKilled(void) const{
    return (killSignal!=SigNone);
  }
  SignalID getKillSignal(void) const{
    return killSignal;
  }
  // Exit this process
  // Returns: true if exit complete, false if process is now zombie
  bool exit(int code);
  // Reap an exited process
  void reap();
  void doKill(SignalID sig){
    I(!isExited());
    I(!isKilled());
    I(sig!=SigNone);
    killSignal=sig;
  }

  // Debugging

  typedef std::vector<VAddr> AddrVector;
  AddrVector entryStack;
  AddrVector raddrStack;
  AddrVector frameStack;
  bool  pendJump;
  VAddr jumpSrc;
  VAddr jumpDst;

  void execCall(VAddr  retAddr, VAddr dstFunc, VAddr sp){
    I(!pendJump);
    entryStack.push_back(dstFunc);
    raddrStack.push_back(retAddr);
    frameStack.push_back(sp);
  }
  void execRet(void){
    I(!pendJump);
    entryStack.pop_back();
    raddrStack.pop_back();
    frameStack.pop_back();
  }
  void execJump(VAddr src, VAddr dst);
  void execInst(VAddr addr, VAddr sp);
  void dumpCallStack(void);
  void clearCallStack(void);

#endif // For (defined MIPS_EMUL)
#if !(defined MIPS_EMUL)
  icode_ptr getPicode() const { return picode; }
  void setPicode(icode_ptr p) {
    picode = p;
  }

  icode_ptr getTarget() const { return target; }
  void setTarget(icode_ptr p) {
    target = p;
  }

  int getperrno() const { return *perrno; }
  void setperrno(int v) {
    I(perrno);
    *perrno = v;
  }

  int getFD(int id) const { return fd[id]; }
  void setFD(int id, int val) {
    fd[id] = val;
  }
  
  static void initMainThread();
#endif // For !(defined MIPS_EMUL)
};

#if !(defined MIPS_EMUL)
typedef ThreadContext mint_thread_t;

// This class helps extract function parameters for substitutions (e.g. in subs.cpp)
// First, prepare for parameter extraction by constructing an instance of MintFuncArgs.
// The constructor takes as a parameter the ThreadContext in which a function has just
// been called (i.e. right after the jal and the delay slot have been emulated
// Then get the parameters in order from first to last, using getInt32 or getInt64
// MintFuncArgs automatically takes care of getting the needed parameter from the register
// or from the stack, according to the MIPS III caling convention. In particular, it correctly
// implements extraction of 64-bit parameters, allowing lstat64 and similar functions to work
// The entire process of parameter extraction does not change the thread context in any way
class MintFuncArgs{
 private:
  const ThreadContext *myContext;
  const icode_t *myIcode;
  int   curPos;
 public:
  MintFuncArgs(const ThreadContext *context, const icode_t *picode)
    : myContext(context), myIcode(picode), curPos(0)
    {
    }
  int getInt32(void);
  long long int getInt64(void);
  VAddr getVAddr(void){ return (VAddr)getInt32(); }
};

#define REGNUM(R) (*((int *) &pthread->reg[R]))
#endif // For !(defined MIPS_EMUL)

#endif // THREADCONTEXT_H
