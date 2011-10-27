#include <nacl/nacl_srpc.h>

NaClSrpcError MyMethod(NaClSrpcChannel *channel,
                       NaClSrpcArg     **in_args,
                       NaClSrpcArg     **out_args)
{
/*
  int v0 = in_args[0]->u.ival;
  int v1 = in_args[1]->u.ival;
  int v2;
  int num = out_args[0]->u.iaval.count;
  int *dest = out_args[0]->u.iaval.iarr;
  int i;

  if (num < 2) return NACL_SRPC_RESULT_APP_ERROR;
  *dest++ = v0;
  *dest++ = v1;
  for (i = 2; i < num; ++i) {
    v2 = v0 + v1;
    *dest++ = v2;
    v0 = v1; v1 = v2;
  }
*/
  return NACL_SRPC_RESULT_OK;
}

// Export MyMethod as fib, which takes two scalar integers and returns an array of integers.
NACL_SRPC_METHOD("fib:ii:I", MyMethod);
