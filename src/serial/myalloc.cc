#ifdef IBM_XLC
  #define _ANSI_HEADER
#endif

#include <ulimit.h>

#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>
#include <assert.h>

#ifdef _ANSI_HEADER
  using namespace std;
  #include <iostream>
  #include <functional>
  #include <deque>
  #include <stack>
  #include <map>
#else
  #include <iostream.h>
  #include <function.h>
  #include <deque.h>
  #include <stack.h>
  #include <map.h>
#endif

#include "myalloc.h"

const long   MyAlloc :: MAX_HOLD_ADD  = 400000 ;  // max MAX_HOLD_ADD Objekte werden gespeichert
const double MyAlloc :: MAX_HOLD_MULT = 1.3 ;     // max das MAX_HOLD_MULT-fache der momentan
                                                  // aktiven Objekte werden gespeichert
long MyAlloc :: _init ;

struct AllocEntry {

  // N verfolgt die Anzahl der angelegten Objekte der entsprechenden
  // Gr"osse, die in Gebrauch sind, auf dem Stack liegen die Objekte,
  // f"ur die delete aufgerufen wurde, die aber nicht an free () zur"uck-
  // gegeben werden um Fragmentierung zu vermeiden.

  long N ;

  stack <void * > S ;

  AllocEntry () : N (0), S () {}
 ~AllocEntry () {
    while (!S.empty ()) {
      free (S.top ()) ;
      S.pop () ;
    }
    return ;
  }
} ;

static map < size_t, AllocEntry, less < size_t > > * freeStore = 0 ;

void * MyAlloc :: operator new (size_t s) throw (OutOfMemoryException) 
{
  assert(s > 0);
  {
    AllocEntry & fs ((*freeStore) [s]) ;
    ++ fs.N ;
    if (fs.S.empty ()) {
      void * p = malloc (s) ;
      if (p == NULL) {
        perror ("**FEHLER (FATAL) in MyAlloc :: operator new ()") ;
        cerr << "**INFO MyAlloc :: operator new (" << s << "): Der gesamte belegte Speicherbereich war " 
#ifdef IBM_XLC
             << (mallinfo ().arena /1024)
#else
             << "[FEHLER: mallinfo() nicht verfuegbar] "
#endif
             << "kb als das Programm out-of-memory lief." << endl ;
        throw OutOfMemoryException () ;
      }
      return p ;
    } else {
      void * p = fs.S.top () ;
      fs.S.pop () ;
      return p ;
    }
  }
}

void MyAlloc :: operator delete (void *ptr, size_t s) {
  AllocEntry & fs ((*freeStore) [s]) ;
  assert (fs.N > 0) ;
  --fs.N ;
  fs.S.push (ptr) ;
  if (fs.S.size () >= (unsigned) MAX_HOLD_ADD
    && double (fs.S.size()) >= MAX_HOLD_MULT * double (fs.N)) {
    assert (!fs.S.empty()) ;
    free (fs.S.top ()) ;
    fs.S.pop() ;
  }
  return ;
}

MyAlloc :: Initializer :: Initializer () {
  if (0 == MyAlloc :: _init ++) {
#ifdef IBM_XLC
    {
      // Auf der SP sollte immer das eingestellte Limit von 143 MB umgangen werden.
      // Der Knoten hat 512 MB, und wird exklusiv benutzt - warum also sparsam sein.
      // Die Programme m"ussen aber auch mit -bmaxdata:#bytes gelinkt werden, um den
      // Platz adressieren zu k"onnen.
    
      long test = ulimit(UL_GETMAXBRK) ;
      if (test == -1) {
        cerr << "**WARNUNG (IGNORIERT) Hochsetzen des Limits f\"ur den Breakvalue hat nicht geklappt. In " << __FILE__ " Zeile " << __LINE__ << endl ;
      } else {
        unsigned long limit = ulimit (GET_DATALIM) ;
        limit /= 0x00100000 ;
        cout << "**INFO MyAlloc :: Initializer :: Initializer () Neues Limit f\"ur den Breakvalue : " << limit << " MBytes." << endl ;
      }
    }
#endif
    freeStore = new map < size_t, AllocEntry, less < size_t > > ;
    assert (freeStore) ;
  }
  return ;
}

MyAlloc :: Initializer :: ~Initializer () {
  if (0 == -- MyAlloc :: _init) 
  {
    if(freeStore) delete freeStore ;
    freeStore = 0 ;
  }
  return ;
}
