#include <string.h>

#include <cstdio>
#include <cstdlib>
using namespace std;
#ifndef potato_h
#define potato_h
class Potato {
public:
  int num_hops;
  int num_path;
  int trace[512];
  Potato():num_hops(0),num_path(0) {
    memset(trace, 0, sizeof(trace));
  }
  void print_Trace() {
    if(num_path > 0){
      cout<<"Trace of potato" << endl;
      for(int i = 0; i < num_path; ++i) {
	if(i!= num_path - 1) cout<< trace[i]<< ",";
	else cout<< trace[i]<< endl;
      }

    }
  }
};

#endif /* potato_h */
