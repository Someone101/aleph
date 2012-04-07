/* ctl_interface.h
 * aleph-avrr32
 *
 * public interface functions for using and manipulating the controller network
 */

#ifndef _CTL_INTERFACE_H_
#define _CTL_INTERFACE_H_

#include "../common/types.h"
#include "op.h"

// maximum allocated IO points and operators
#define CTLNET_INS_MAX 128
#define CTLNET_OUTS_MAX 128
#define CTLNET_OPS_MAX 128

//---- public functions

// initialize the network 
void net_init(void);
// de-initialize the network 
void net_deinit(void);
// create a new operator given class ID, return index (-1 == fail)
S16 net_add_op(opid_t opId);
// remove the last created operator
S16 net_pop_op(void);
// remove an arbitrary operator
// TODO
// void remove_op(const U8 idx);
// activate an input node with some input data
void net_activate(S16 inIdx, const S32* val);
// get current count of operators
U8 net_num_ops(void);
// get current count of inputs
U8 net_num_ins(void);
// get current count of outputs
U8 net_num_outs(void);
// get string for operator at given idx
const char* net_op_name(const U8 idx);
// get name for input at given idx
const char* net_in_name(const U8 idx);
// get name for output at given idx
const char* net_out_name(const U8 idx);
// get op index for input at given idx
U8 net_in_op_idx(const U8 idx);
// get op index for output at given idx
U8 net_out_op_idx(const U8 idx);

// connect a given output and input idx pair
void net_connect(U32 outIdx, U32 inIdx);
// disconnect a given output
void net_disconnect(U32 outIdx);

// populate an array with indices of all connected outputs for a given index
// returns count of connections
U32 net_gather(U32 iIdx, U32(*outs)[CTLNET_OUTS_MAX]);

#endif // header guard
