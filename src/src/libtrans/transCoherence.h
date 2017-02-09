/**
 * @file
 * @author  jpoe   <>, (C) 2008, 2009
 * @date    09/19/08
 * @brief   This is the interface for the global coherence module.
 *
 * @section LICENSE
 * Copyright: See COPYING file that comes with this distribution
 *
 * @section DESCRIPTION
 * C++ Interface: transCoherence \n
 * This object provides the functional coherence. 
 *
 * @note
 * Commits/Aborts have two phases.  The first induces the stall, the second is where all of the
 * memory addresses are freed.
 */
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

#ifndef TRANSACTION_COHERENCE
#define TRANSACTION_COHERENCE

#include <map>
#include <set>
#include "icode.h"

#define MAX_CPU_COUNT 2048

using namespace std;

enum GCMRet { SUCCESS, NACK, ABORT, IGNORE, COMMIT_DELAY, BACKOFF };
enum condition {INVALID, RUNNING, NACKED, ABORTING, ABORTED, COMMITTING, COMMITTED, DOABORT};
enum perState { I, W, R };

typedef uintptr_t RAddr;
typedef struct icode *icode_ptr;
typedef unsigned long long Time_t;

struct GCMFinalRet{
  GCMRet ret;
  int writeSetSize;
  int abortCount;
  long long tuid;

   //!< This allows tagging of DINST instructions with information as to whether the transaction is new, replayed, or subsumed
  int BCFlag;
};

struct cacheState{
  perState state;
  set<int> readers;
  set<int> writers;
};

struct tmState{
  condition state;
  Time_t timestamp;
  int cycleFlag;
  long long utid;
  RAddr beginPC;
};

/**
 * @ingroup transCoherence
 * @brief   TM Coherency Manager
 *
 * Coordinates the entire coherency of the transactional memory system. Read/Write/Abort/Commit/Begin
 * must all be provided and linked to functional pointers at runtime to determine EE/LL/etc.
 */
class transCoherence{
  public:
    // Constructor
    transCoherence();
    transCoherence(FILE *out, int conflicts, int versioning, int cacheLineSize);

    GCMRet readEE(int pid, int tid, RAddr raddr);
    GCMRet writeEE(int pid, int tid, RAddr raddr);
    struct GCMFinalRet abortEE(thread_ptr pthread, int tid);
    struct GCMFinalRet commitEE(int pid, int tid);
    struct GCMFinalRet beginEE(int pid, icode_ptr picode);

    GCMRet readLL(int pid, int tid, RAddr raddr);
    GCMRet writeLL(int pid, int tid, RAddr raddr);
    struct GCMFinalRet abortLL(thread_ptr pthread, int tid);
    struct GCMFinalRet commitLL(int pid, int tid);
    struct GCMFinalRet beginLL(int pid, icode_ptr picode);

    GCMRet read(int pid, int tid, RAddr raddr);
    GCMRet write(int pid, int tid, RAddr raddr);
    struct GCMFinalRet abort(thread_ptr pthread, int tid);
    struct GCMFinalRet commit(int pid, int tid);
    struct GCMFinalRet begin(int pid, icode_ptr picode);

    GCMRet (transCoherence::*readPtr)(int pid, int tid, RAddr raddr);
    GCMRet (transCoherence::*writePtr)(int pid, int tid, RAddr raddr);
    struct GCMFinalRet (transCoherence::*abortPtr)(thread_ptr pthread, int tid);
    struct GCMFinalRet (transCoherence::*commitPtr)(int pid, int tid);
    struct GCMFinalRet (transCoherence::*beginPtr)(int pid,icode_ptr picode);

    bool checkAbort(int pid, int tid);
    int  getVersioning();

    void stallUntil(int cpu,Time_t stall){
      stallCycle[cpu] = globalClock + stall;
    }

    bool checkStall(int cpu){
      if(cpu < 0)
        return false;
      else
        return stallCycle[cpu] >= globalClock;
    }

    bool checkStallState(int cpu)
    {
      return transState[cpu].state == NACKED;
    }


  private:

    RAddr addrToCacheLine(RAddr raddr);
    struct cacheState newReadState(int pid);
    struct cacheState newWriteState(int pid);

    int conflictDetection;
    int versioning;
    int cacheLineSize;

    int tmDepth[MAX_CPU_COUNT];

    int abortCount[MAX_CPU_COUNT];

    pair<int,RAddr> abortReason[MAX_CPU_COUNT];

    Time_t stallCycle[MAX_CPU_COUNT];

    int currentCommitter;                          //!< PID of the currently committing processor

    long long int utid;                            //!< Unique Global Transaction ID

    FILE *out;

    map<RAddr, cacheState>     permCache;          //!< The cache ownership
    struct tmState             transState[MAX_CPU_COUNT];
};


inline RAddr transCoherence::addrToCacheLine(RAddr raddr){
    while(raddr%cacheLineSize != 0)
      raddr = raddr-1;
  return raddr;
}

inline GCMRet transCoherence::read(int pid, int tid, RAddr raddr){
  return (this->*readPtr)(pid, tid, raddr);
}

inline GCMRet transCoherence::write(int pid, int tid, RAddr raddr){
  return (this->*writePtr)(pid, tid, raddr);
}

inline struct GCMFinalRet transCoherence::abort(thread_ptr pthread, int tid){
  return (this->*abortPtr)(pthread, tid);
}

inline struct GCMFinalRet transCoherence::commit(int pid, int tid){
  return (this->*commitPtr)(pid, tid);
}

inline struct GCMFinalRet transCoherence::begin(int pid, icode_ptr picode){
  return (this->*beginPtr)(pid, picode);
}

inline int transCoherence::getVersioning(){
  return versioning;
}

extern transCoherence *transGCM;
extern Time_t globalClock;
#endif

/**
 * @struct  GCMFinalRet
 * @ingroup transCoherence
 * @brief   Structure used to store result of coherence request
 *
 * This is needed because we need additional information
 * about the write set size for aborts/commits
 */

/**
 * @struct  cacheState
 * @ingroup transCoherence
 * @brief   Cache Line State Container (R/W)
 */

/**
 * @struct  tmState
 * @ingroup transCoherence
 * @brief   TM State Container
 */

/**
 * @enum GCMRet
 * Global coherency module return values.
 */

/**
 * @enum condition
 * Transaction conditions.
 */

/**
 * @enum perState
 * Cache line state values.
 */

/**
 * @typedef RAddr
 * uintptr_t.
 */

/**
 * @typedef Time_t
 * unsigned long long
 */

/**
 * @typedef *icode_ptr
 * icode
 */
