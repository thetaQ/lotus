#ifndef _LOTUS_GLOBAL_H_
#define _LOTUS_GLOBAL_H_

#define ERR(eval) if(eval < 0){perror(#eval); exit(-1);}
#define ERR2(res, eval) if((res = eval) < 0){perror(#eval); exit(-1);}

#endif
