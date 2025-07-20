#ifndef __CASYNCIO_H__
#define __CASYNCIO_H__

/* This headerfile contains various components of different versions of the
 * CPython asynciomodule.c in order to make the objects run a bit smoother.
 * In Cython, Cython Exposes arrary.array under a hard-coded utility hack
 * Likewise The plan is to do that in a similar manner but since the amount
 * of functions we wish to optimize and sharpen is pretty chaotic it made
 * Sense not to just put them in a pxd file alone and call it a day.
 *
 * Future Objects and Task Objects are all over the place when we want to
 * start accounting for all supported versions of CPython that still get
 * security updates
 *
 *
 * Since the project is Technically MIT/APPACHE 2.0 Which CPython is APPACHE 2.0
 * There should be no problems with borrowing that portion of code in here to
 * improve the performance of winloop or uvloop if uvloop deicded to borrow from me
 * feel free to take whatever you need from me :)
 *
 *
 * Hopfully within the next 5 - 10 years this encourages the CPython maintainers to
 * Make a CAPI Capsule for us to use but we shall see...
 */

/* List of Items that should be in this custom CAPI are items
 * That must Eclipse over module "_asyncio" not "asyncio"
 *
 * These include
 *
 * - Future Object
 *   - Most of the functions are remade to return integers
 *     instead of Python Objects for speed. 
 * - Task Object
 *   - More or less the same idea
 * */

/*
 * It is on my todolist to transform parts into a capsule styled apporch for the global objects
 */


#include <Python.h>
#include <stdint.h>

#include "pycore_pyerrors.h" // _PyErr_ClearExcState()
#include <stddef.h>          // offsetof()

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum {
    STATE_PENDING,
    STATE_CANCELLED,
    STATE_FINISHED
} fut_state;

// Different Python versions Laid out as Hex Values
// I made Macros to simplify saying if 3.XX >= ... or 3.XX+

#if (PY_VERSION_HEX >= 0x030a00f0)
#define CASYNCIO_PYTHON_310
#endif
#if (PY_VERSION_HEX >= 0x030b00f0)
#define CASYNCIO_PYTHON_311
#endif
#if (PY_VERSION_HEX >= 0x030c00f0)
#define CASYNCIO_PYTHON_312
#endif
#if (PY_VERSION_HEX >= 0x030d00f0)
#define CASYNCIO_PYTHON_313
#endif
#if (PY_VERSION_HEX >= 0x030e00f0)
#define CASYNCIO_PYTHON_314
#endif

/* XXX Can't use macros on FutureObj_HEAD Due to the way C Macros work but we can hardcode it in still */
/* We can copy and paste this work into TaskObj */

typedef struct
{
    PyObject_HEAD
#ifndef CASYNCIO_PYTHON_313
    PyObject *fut_loop;
    PyObject *fut_callback0;
    PyObject *fut_context0;
    PyObject *fut_callbacks;
    PyObject *fut_exception;
#ifdef CASYNCIO_PYTHON_310
    PyObject *fut_exception_tb;
#endif
    PyObject *fut_result;
    PyObject *fut_source_tb;
    PyObject *fut_cancel_msg;
    fut_state fut_state;
    int fut_log_tb;
    int fut_blocking;
    PyObject *dict;
    PyObject *fut_weakreflist;
#if CASYNCIO_PYTHON_311
    PyObject *fut_cancelled_exc;
#else
    /* XXX: Altered after 3.10 retained newer name to be simplistic */
    _PyErr_StackItem fut_cancelled_exc;
#endif /* CASYNCIO_PYTHON_311 */

#else 
    /* 313 Takes a Massive leap that is too impressive to organize */
    PyObject *fut_loop;
    PyObject *fut_callback0;
    PyObject *fut_context0;
    PyObject *fut_callbacks;
    PyObject *fut_exception;
    PyObject *fut_exception_tb;
    PyObject *fut_result;
    PyObject *fut_source_tb;
    PyObject *fut_cancel_msg;
    PyObject *fut_cancelled_exc;
    fut_state fut_state;
    /* These bitfields need to be at the end of the struct
       so that these and bitfields from TaskObj are contiguous.
    */
    unsigned fut_log_tb : 1;
    unsigned fut_blocking : 1;

#endif /* CASYNCIO_PYTHON_313 */
} FutureObj;

typedef struct {
#ifndef CASYNCIO_PYTHON_313
    PyObject *task_loop;
    PyObject *task_callback0;
    PyObject *task_context0;
    PyObject *task_callbacks;
    PyObject *task_exception;
#ifdef CASYNCIO_PYTHON_310
    PyObject *task_exception_tb;
#endif
    PyObject *task_result;
    PyObject *task_source_tb;
    PyObject *task_cancel_msg;
    fut_state task_state;
    int task_log_tb;
    int task_blocking;
    PyObject *dict;
    PyObject *task_weakreflist;
#if CASYNCIO_PYTHON_311
    PyObject *task_cancelled_exc;
#else
    /* XXX: Altered after 3.10 */
    _PyErr_StackItem task_cancelled_exc;
#endif /* CASYNCIO_PYTHON_311 */

#else 
    /* 313 Takes a Massive leap that is too impressive to organize */
    PyObject *task_loop;
    PyObject *task_callback0;
    PyObject *task_context0;
    PyObject *task_callbacks;
    PyObject *task_exception;
    PyObject *task_exception_tb;
    PyObject *task_result;
    PyObject *task_source_tb;
    PyObject *task_cancel_msg;
    PyObject *task_cancelled_exc;
    fut_state task_state;
    /* These bitfields need to be at the end of the struct
       so that these and bitfields from TaskObj are contiguous.
    */
    unsigned task_log_tb : 1;
    unsigned task_blocking : 1;

#endif /* CASYNCIO_PYTHON_313 */
#ifndef CASYNCIO_PYTHON_313
    PyObject *task_fut_waiter;
    PyObject *task_coro;
    PyObject *task_name;
    PyObject *task_context;
    int task_must_cancel;
    int task_log_destroy_pending;
#if CASYNCIO_PYTHON_311
    int task_num_cancels_requested;
#endif
#else 
    /* 3.13 still takes massive leaps and makes crazy changes */
    unsigned task_must_cancel: 1;
    unsigned task_log_destroy_pending: 1;
    int task_num_cancels_requested;
    PyObject *task_fut_waiter;
    PyObject *task_coro;
    PyObject *task_name;
    PyObject *task_context;
#endif 
} TaskObj;



/* 
Going to deviate from CPython's implementations a little bit Cython
Can Strictly take care of preventing unwated types 
from leaking through

- This C-API Assumes that the object 
  is that type & it exists & Cython
  will recast it as that object by 
  default.


*/

// Couldn't seem to get _Py_IDENTIFIER to work correctly
// so I had to come up with a comprimise.

#define CASYNCIO_IDENTIFIER(name) static _Py_Identifier PyId_##name = {#name, -1}

// ==================================== GLOBALS ====================================

// TODO: Any globals we borrow should be setup in a hacky import_XX function later...
PyObject* context_kwname;
PyObject* asyncio_InvalidStateError;
PyObject* asyncio_CancelledError;


PyTypeObject* FutureType; /* _asyncio.Future */
PyTypeObject* TaskType; /* _asyncio.Task */

CASYNCIO_IDENTIFIER(call_soon);

// TODO: Check 3.9-3.13 for anything else or code-changes


static int 
Loop_CallSoon(PyObject* loop, PyObject* func, PyObject* arg, PyObject* ctx){
#ifndef CASYNCIO_PYTHON_311
    PyObject *handle;
    PyObject *stack[3];
    Py_ssize_t nargs;

    if (ctx == NULL) {
        handle = _PyObject_CallMethodIdObjArgs(
            loop, &PyId_call_soon, func, arg, NULL);
    }
    else {
        /* Use FASTCALL to pass a keyword-only argument to call_soon */

        PyObject *callable = _PyObject_GetAttrId(loop, &PyId_call_soon);
        if (callable == NULL) {
            return -1;
        }

        /* All refs in 'stack' are borrowed. */
        nargs = 1;
        stack[0] = func;
        if (arg != NULL) {
            stack[1] = arg;
            nargs++;
        }
        stack[nargs] = (PyObject *)ctx;

        handle = PyObject_Vectorcall(callable, stack, nargs, context_kwname);
        Py_DECREF(callable);
    }

    if (handle == NULL) {
        return -1;
    }
    Py_DECREF(handle);
    return 0;
#else /* 3.12+ */
    /* IDK if _Py_ID or any functionality is Private to CPython only 
       Feel free to throw an issue on github if you find something 
       Unaccessable or requires new objects or macros to be added.
    */
    PyObject *handle;

    if (ctx == NULL) {
        PyObject *stack[] = {loop, func, arg};
        size_t nargsf = 3 | PY_VECTORCALL_ARGUMENTS_OFFSET;
        handle = PyObject_VectorcallMethod(&_Py_ID(call_soon), stack, nargsf, NULL);
    }
    else {
        /* All refs in 'stack' are borrowed. */
        PyObject *stack[4];
        size_t nargs = 2;
        stack[0] = loop;
        stack[1] = func;
        if (arg != NULL) {
            stack[2] = arg;
            nargs++;
        }
        stack[nargs] = (PyObject *)ctx;
        size_t nargsf = nargs | PY_VECTORCALL_ARGUMENTS_OFFSET;
        handle = PyObject_VectorcallMethod(&_Py_ID(call_soon), stack, nargsf,
                                           context_kwname);
    }

    if (handle == NULL) {
        return -1;
    }
    Py_DECREF(handle);
    return 0;
#endif

}

// NOTE: FutureObj_GetLoop could be inlined in a pxd file so I didn't write it
// If you want me to rewrite and addd it back in throw me an issue on github
// if you wanted to borrow it for anything else...


static inline int FutureObj_IsAlive(FutureObj* fut)
{
    return fut->fut_loop != NULL;
}

static inline int FutureObj_EnsureAlive(FutureObj *fut)
{
    if (!FutureObj_EnsureAlive(fut)) {
        PyErr_SetString(PyExc_RuntimeError,
                        "Future object is not initialized.");
        return -1;
    }
    return 0;
}

static int 
FutureObj_ScheduleCallbacks(FutureObj* fut){
    Py_ssize_t len;
    Py_ssize_t i;

    if (fut->fut_callback0 != NULL) {
        /* CPYTHON COMMENT: There's a 1st callback */
        #ifdef CASYNCIO_PYTHON_312
        /* 3.12+ */

        // CPYTHON WARNING! 
        // Beware: An evil call_soon could alter fut_callback0 or fut_context0.
        // Since we are anyway clearing them after the call, whether call_soon
        // succeeds or not, the idea is to transfer ownership so that external
        // code is not able to alter them during the call.
        PyObject *fut_callback0 = fut->fut_callback0;
        fut->fut_callback0 = NULL;
        PyObject *fut_context0 = fut->fut_context0;
        fut->fut_context0 = NULL;

        // XXX: We don't utilize a state since 
        // were borrowing to hack in optimized code.
        int ret = Loop_CallSoon(
            fut->fut_loop, fut_callback0,
            (PyObject *)fut, fut_context0);
        
        
        #else
        /* 3.9 - 3.11 */
        int ret = Loop_CallSoon(
            fut->fut_loop, fut->fut_callback0,
            (PyObject *)fut, fut->fut_context0);
        
        #endif /* CASYNCIO_PYTHON_312 */


        Py_CLEAR(fut->fut_callback0);
        Py_CLEAR(fut->fut_context0);
        if (ret) {
            /* If an error occurs in pure-Python implementation,
               all callbacks are cleared. */
            Py_CLEAR(fut->fut_callbacks);
            return ret;
        }

        /* we called the first callback, now try calling
           callbacks from the 'fut_callbacks' list. */
    }

    if (fut->fut_callbacks == NULL) {
        /* CPYTHON COMMENT: No more callbacks, return. */
        return 0;
    }

    #ifndef CASYNCIO_PYTHON_312

    /* 3.9 - 3.11 */

    len = PyList_GET_SIZE(fut->fut_callbacks);
    if (len == 0) {
        /* CPYTHON COMMENT: The list of callbacks was empty; clear it and return. */
        Py_CLEAR(fut->fut_callbacks);
        return 0;
    }

    for (i = 0; i < len; i++) {
        PyObject *cb_tup = PyList_GET_ITEM(fut->fut_callbacks, i);
        PyObject *cb = PyTuple_GET_ITEM(cb_tup, 0);
        PyObject *ctx = PyTuple_GET_ITEM(cb_tup, 1);

        if (Loop_CallSoon(fut->fut_loop, cb, (PyObject *)fut, ctx)) {
            /* If an error occurs in pure-Python implementation,
               all callbacks are cleared. */
            Py_CLEAR(fut->fut_callbacks);
            return -1;
        }
    }
    Py_CLEAR(fut->fut_callbacks);
    return 0; 
    #else 
    /* ===== 3.12+ ===== */
    
    // CPYTHON WARNING! 
    // Beware: An evil call_soon could change fut->fut_callbacks.
    // The idea is to transfer the ownership of the callbacks list
    // so that external code is not able to mutate the list during
    // the iteration.
    PyObject *callbacks = fut->fut_callbacks;
    fut->fut_callbacks = NULL;
    Py_ssize_t n = PyList_GET_SIZE(callbacks);
    for (Py_ssize_t i = 0; i < n; i++) {
        assert(PyList_GET_SIZE(callbacks) == n);
        PyObject *cb_tup = PyList_GET_ITEM(callbacks, i);
        PyObject *cb = PyTuple_GET_ITEM(cb_tup, 0);
        PyObject *ctx = PyTuple_GET_ITEM(cb_tup, 1);

        if (Loop_CallSoon(state, fut->fut_loop, cb, (PyObject *)fut, ctx)) {
            Py_DECREF(callbacks);
            return -1;
        }
    }
    Py_DECREF(callbacks);
    return 0;
    
    #endif
}

// one change I did make was that some return types are integers instead of PyObjects
// so that a few extra performance modifications could be made later...

static int
FutureObj_SetResult(FutureObj *fut, PyObject *res)
{
    if (FutureObj_EnsureAlive(fut)) {
        return -1;
    }

    if (fut->fut_state != STATE_PENDING) {
        PyErr_SetString(asyncio_InvalidStateError, "invalid state");
        return -1;
    }
    assert(!fut->fut_result);

    #ifndef CASYNCIO_PYTHON_312
    /* 3.9 - 3.11 */
    Py_INCREF(res);
    fut->fut_result = res;
    #else /* 3.12+ */
    fut->fut_result = Py_NewRef(res);
    #endif
    fut->fut_state = STATE_FINISHED;
    if (FutureObj_ScheduleCallbacks(fut) == -1) {
        return -1;
    }
    return 0;
}



static int 
FutureObj_SetException(FutureObj *fut, PyObject *exc)
{
    PyObject *exc_val = NULL;

    if (fut->fut_state != STATE_PENDING) {
        PyErr_SetString(asyncio_InvalidStateError, "invalid state");
        return -1;
    }

    if (PyExceptionClass_Check(exc)) {
        exc_val = PyObject_CallNoArgs(exc);
        if (exc_val == NULL) {
            return -1;
        }
        if (fut->fut_state != STATE_PENDING) {
            Py_DECREF(exc_val);
            PyErr_SetString(asyncio_InvalidStateError, "invalid state");
            return -1;
        }
    }
    else {
        exc_val = exc;
        Py_INCREF(exc_val);
    }
    if (!PyExceptionInstance_Check(exc_val)) {
        Py_DECREF(exc_val);
        PyErr_SetString(PyExc_TypeError, "invalid exception object");
        return NULL;
    }
    if (Py_IS_TYPE(exc_val, (PyTypeObject *)PyExc_StopIteration)) {
        Py_DECREF(exc_val);
        PyErr_SetString(PyExc_TypeError,
                        "StopIteration interacts badly with generators "
                        "and cannot be raised into a Future");
        return -1;
    }

    assert(!fut->fut_exception);
    assert(!fut->fut_exception_tb);
    fut->fut_exception = exc_val;
    fut->fut_exception_tb = PyException_GetTraceback(exc_val);
    fut->fut_state = STATE_FINISHED;

    if (FutureObj_ScheduleCallbacks(fut) == -1) {
        return -1;
    }

    fut->fut_log_tb = 1;
    return 0;
}

/* Combined create_cancelled_error + future_set_cancelled_error 
 * to be more optimized and organized */

static void
FutureObj_SetCancelledError(FutureObj* fut){

   
    #ifdef CASYNCIO_PYTHON_311
    /* 3.11+ */
    PyObject *exc;
    if (fut->fut_cancelled_exc != NULL) {
        /* transfer ownership */
        exc = fut->fut_cancelled_exc;
        fut->fut_cancelled_exc = NULL;
        return exc;
    }
    PyObject *msg = fut->fut_cancel_msg;
    if (msg == NULL || msg == Py_None) {
        exc = PyObject_CallNoArgs(asyncio_CancelledError);
    } else {
        exc = PyObject_CallOneArg(asyncio_CancelledError, msg);
    }
    if (exc == NULL){
        return;
    }
    PyErr_SetObject(asyncio_CancelledError, exc);
    Py_DECREF(exc);

    #else /* 3.9 - 3.10 */
    PyObject *exc, *msg;
    msg = fut->fut_cancel_msg;
    if (msg == NULL || msg == Py_None) {
        exc = PyObject_CallNoArgs(asyncio_CancelledError);
    } else {
        exc = PyObject_CallOneArg(asyncio_CancelledError, msg);
    }
    PyErr_SetObject(asyncio_CancelledError, exc);
    /* were done using the exception object so release... */
    Py_DECREF(exc);
    _PyErr_ChainStackItem(&fut->fut_cancelled_exc);
    #endif
}

static int 
FutureObj_GetResult(FutureObj* fut, PyObject** result){
    // made more sense to use a switchcase block here than what CPython has.
    switch (fut->fut_state)
    {
        case STATE_CANCELLED: {
            FutureObj_SetCancelledError(fut);
            return -1;
        } 

        case STATE_PENDING: {
            PyErr_SetString(asyncio_InvalidStateError, "Result is not set.");
            return -1;
        }
        
        case STATE_FINISHED: {
            fut->fut_log_tb = 0;
            if (fut->fut_exception != NULL){
                Py_INCREF(fut->fut_exception);
                *result = fut->fut_exception;
                return 1;
            }
            Py_INCREF(fut->fut_result);
            *result = fut->fut_result;
            return 0;
        }
        default:
            /* UNREACHABLE, IF REACHED, YOU SCREWED UP BADLY!!! */
            assert(0);
    }
}

static int
FutureObj_AddDoneCallback(FutureObj *fut, PyObject *arg, PyObject *ctx)
{
    if (!FutureObj_IsAlive(fut)) {
        PyErr_SetString(PyExc_RuntimeError, "uninitialized Future object");
        return -1;
    }
    if (fut->fut_state != STATE_PENDING) {
        /* CPYTHON COMMENT: The future is done/cancelled, so schedule the callback
           right away. */
        if (Loop_CallSoon(fut->fut_loop, arg, (PyObject*) fut, ctx)) {
            return -1;
        }
    } else {
        // Large Comment was kind of ignorable I would guess you understand 
        // that the first callback is the optimized object otherwise 
        // the list takes it's place
        if (fut->fut_callbacks == NULL && fut->fut_callback0 == NULL) {
            Py_INCREF(arg);
            fut->fut_callback0 = arg;
            Py_INCREF(ctx);
            fut->fut_context0 = ctx;
        }
        else {
            PyObject *tup = PyTuple_New(2);
            if (tup == NULL) {
                return -1;
            }
            Py_INCREF(arg);
            PyTuple_SET_ITEM(tup, 0, arg);
            Py_INCREF(ctx);
            PyTuple_SET_ITEM(tup, 1, (PyObject *)ctx);

            if (fut->fut_callbacks != NULL) {
                int err = PyList_Append(fut->fut_callbacks, tup);
                if (err) {
                    Py_DECREF(tup);
                    return -1;
                }
                Py_DECREF(tup);
            }
            else {
                fut->fut_callbacks = PyList_New(1);
                if (fut->fut_callbacks == NULL) {
                    return -1;
                }

                PyList_SET_ITEM(fut->fut_callbacks, 0, tup);  /* borrow */
            }
        }
    }
    return 0;
}

// Will use numbers for these ones but basically it goes like this...
// 1 = success
// 0 = fail
// -1 = error
static int 
FutureObj_Cancel(FutureObj* fut, PyObject* msg){
    fut->fut_log_tb = 0;

    if (fut->fut_state != STATE_PENDING) {
        return 0;
    }
    fut->fut_state = STATE_CANCELLED;
    Py_XINCREF(msg);
    Py_XSETREF(fut->fut_cancel_msg, msg);

    return (FutureObj_ScheduleCallbacks(fut) < 0) ? -1: 1;
}

// This function is a combined version of __new__ & __init__
// It's assumed asynciomodule.c already tied these objects together.
// This will also take care of the costly calls & globals and lets asynciomodule.c 
// do the heavy lifiting for us. 
// WARNING: IF LOOP IS NULL THIS FUNCTION IS COSTLY AND MAY CAP PERFORAMANCE BENEFITS!!!

static PyObject* 
FutureObj_New(PyObject* loop){
    PyObject* fut = FutureType->tp_alloc(FutureType, 0);
    if (fut == NULL) {
        return NULL;
    }
    Py_INCREF(fut);
    if (loop == NULL){
        return (FutureType->tp_init(fut, NULL, NULL) < 0) ? NULL : fut;
    }
    PyObject* args = PyTuple_Pack(1, loop);
    if (args == NULL){
        /* FAIL */
        goto fail;
    }
    Py_INCREF(args);
    int ret = FutureType->tp_init(fut, args, NULL);
    Py_CLEAR(args);
    if (ret < 0){
        /* Init Error */
        goto fail;
    }
    /* Success */
    return fut;
fail:
    Py_CLEAR(fut);
    return NULL;
}

// Returns -1 if future is NULL as an aggressive safety-measure
static int 
FutureObj_IsDone(FutureObj* fut){
    return (fut == NULL) ? -1 : fut->fut_state == STATE_FINISHED;
}

static int 
FutureObj_IsCancelled(FutureObj* fut){
    return (fut == NULL) ? -1 : fut->fut_state == STATE_CANCELLED;
}


 


#ifdef __cplusplus
}
#endif

#endif // __CASYNCIO_H__
