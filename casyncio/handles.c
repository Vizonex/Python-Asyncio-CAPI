/* This is a concept for `asyncio.Handle` based off version 3.10
and making it run a bit faster as well as allowing for
specialized bindings in Cython

Author: Vizonex
Year: 2025
License: Appache 2.0 / MIT
*/
#include <Python.h>

#include "format_helpers.h"
#include "pythoncapi_compat.h"

/* This also includes a concept for format_helpers since it's involved with
 * handles greatly */

typedef struct _handle_object {
  PyObject_HEAD PyObject *_callback;
  PyObject *_args;
  int _cancelled;
  PyObject *_loop;
  PyObject *_repr;
  PyObject *_source_traceback;
  PyObject *_context;
#if PY_VERSION_HEX >= 0x030c00f0
  PyObject *weaklist; /* __weakref__ */
#endif
} HandleObject;

typedef struct _timer_handle_object {
  PyObject_HEAD PyObject *_callback;
  PyObject *_args;
  int _cancelled;
  PyObject *_loop;
  PyObject *_repr;
  PyObject *_source_traceback;
  PyObject *_context;
#if PY_VERSION_HEX >= 0x030c00f0
  PyObject *weaklist; /* __weakref__ */
#endif
  int _scheduled;
  /* When could be either integers or floats hence the implementation here */
  PyObject *_when;
} TimerHandleObject;

PyTypeObject Handle_Type;
PyTypeObject TimerHandle_Type;

#define HANDLE_XSETATTR(ITEM, ITEM2) Py_XSETREF(ITEM, Py_NewRef(ITEM2))

// Couldn't seem to get _Py_IDENTIFIER to work correctly
// so I had to come up with a comprimise.

#define HANDLE_IDENTIFIER(name) static _Py_Identifier PyId_##name = {#name, -1}
HANDLE_IDENTIFIER(run);
HANDLE_IDENTIFIER(get_debug);
HANDLE_IDENTIFIER(call_exception_handler);
HANDLE_IDENTIFIER(_timer_handle_cancelled);

static inline int Loop_GetDebug(PyObject *loop) {
  PyObject *get_debug = _PyObject_GetAttrId(loop, &PyId_get_debug);
  if (get_debug == NULL) {
    return -1;
  }
  PyObject *debug_ret = PyObject_CallNoArgs(get_debug);
  if (debug_ret == NULL) {
    return -1;
  };
  int is_true = PyObject_IsTrue(debug_ret);
  Py_XDECREF(debug_ret);
  return is_true;
}

static inline int Loop__timer_handle_cancelled(PyObject *loop,
                                               PyObject *handle) {
  PyObject *func = _PyObject_GetAttrId(loop, &PyId__timer_handle_cancelled);
  if (func == NULL) {
    return -1;
  }
  PyObject *debug_ret = PyObject_CallOneArg(func, handle);
  if (debug_ret == NULL) {
    return -1;
  };
  return 0;
}

static inline int Loop_call_exception_handler(PyObject *loop,
                                              PyObject *context) {
  PyObject *func = _PyObject_GetAttrId(loop, &PyId_call_exception_handler);
  if (func == NULL) {
    return -1;
  }
  PyObject *debug_ret = PyObject_CallOneArg(func, context);
  if (debug_ret == NULL) {
    return -1;
  };
  int is_true = PyObject_IsTrue(debug_ret);
  Py_XDECREF(debug_ret);
  return is_true;
}

/* Subclassable c-api _handle_init which gets used several different times */
static int _handle_init(HandleObject *self, PyObject *context, PyObject *args,
                        PyObject *loop, PyObject *callback) {
  /* Kept some compatability from the Python version since the only goal is to
   * internally refactor */
  self->_context = (context == NULL || Py_IsNone(context))
                       ? PyContext_CopyCurrent()
                       : Py_NewRef(context);
  if (self->_context == NULL) {
    return -1;
  }
  HANDLE_XSETATTR(self->_loop, loop);
  HANDLE_XSETATTR(self->_callback, callback);
  HANDLE_XSETATTR(self->_args, args);

  self->_cancelled = 0;
  self->_repr = Py_NewRef(Py_None);

  int debug = Loop_GetDebug(self->_loop);
  if (debug < 0) {
    return -1;
  }
  if (debug) {
    // For Right now I will not worry about this or let the
    // FormatHelpers module be the one in charge of this and skip to the more
    // important parts of this concept code. pass for now...
    self->_source_traceback = Py_NewRef(Py_None);
  } else {
    self->_source_traceback = Py_NewRef(Py_None);
  }
  return 0;
}

/* Handle.__init__ */
static int Handle_tp_init(HandleObject *self, PyObject *args, PyObject *kwds) {
  static char *kwlist[] = {"callback", "args", "loop", "context", NULL};
  PyObject *_callback = NULL, *_args = NULL, *_loop = NULL, *context = NULL;
  if (PyArg_ParseTupleAndKeywords(args, kwds, "OOO|O", kwlist, _callback, _args,
                                  _loop, context) < 0) {
    return -1;
  };

  return 0;
}

/* Handle.cancel() */
static int _handle_cancel(HandleObject *self) {
  if (!self->_cancelled) {
    self->_cancelled = 1;
    int debug = Loop_GetDebug(self->_loop);
    switch (Loop_GetDebug(self->_loop)) {
      case -1:
        return -1;
      case 1: {
        self->_repr = PyObject_Repr((PyObject *)self);
        break;
      }
      default:
        break;
    }
    /* Py_None Prevents Segfaults */
    Py_CLEAR(self->_callback);
    self->_callback = Py_NewRef(Py_None);
    Py_CLEAR(self->_args);
    self->_args = Py_NewRef(Py_None);
  }
  return 0;
}
/* returns self.cancelled as a C integer */
static int _handle_is_cancelled(HandleObject *self) { return self->_cancelled; }

PyObject *Handle_cancelled_impl(HandleObject *self) {
  return PyBool_FromLong(self->_cancelled);
}

/* Handle._run() */
static int Handle__run_impl(HandleObject *self) {
  PyObject *run_func = _PyObject_GetAttrId(self->_context, &PyId_run);
  if (run_func == NULL) {
    /* For C-API incase Context is somehow hacked by the end developer */
    PyErr_SetString(PyExc_AttributeError,
                    "Context has no function named \"run\"");
    return -1;
  }
  PyObject *ret = PyObject_Call(run_func, self->_args, NULL);
  if (ret == NULL) {
    if (PyErr_ExceptionMatches(PyExc_SystemExit) ||
        PyErr_ExceptionMatches(PyExc_KeyboardInterrupt)) {
      /* user wants to exit or SystemExit occured */
      return -1;
    }
    /* except BaseException as exc */
    /* so at this point we need to format the callback source and
    then throw it into a context to be called later...
    here's a C Version of _format_callback_source, TODO: _get_function_source */
    PyObject *msg = PyUnicode_FromStringAndSize("Exception in callback ", 23);
    PyUnicode_AppendAndDel(
        &msg, _format_callback_source(self->_callback, self->_args));
    PyObject *context = PyDict_New();
    PyDict_SetItemString(context, "message", msg);
    PyDict_SetItemString(context, "exc", PyErr_Occurred());
    PyDict_SetItemString(context, "handle", Py_NewRef(self));
    if (!(self->_source_traceback == NULL ||
          Py_IsNone(self->_source_traceback))) {
    }
    if (Loop_call_exception_handler(self->_loop, context) < 0) {
      return -1;
    }
    Py_XDECREF(self);
    return -1;
  }
  return 0;
}

static int Handle_tp_clear(HandleObject *self) {
  Py_CLEAR(self->_args);
  Py_CLEAR(self->_callback);
  Py_CLEAR(self->_source_traceback);
  Py_CLEAR(self->_context);
  Py_CLEAR(self->_loop);
  Py_CLEAR(self->_repr);
  return 0;
}

static int Handle_tp_traverse(HandleObject *self, visitproc visit, void *arg) {
  Py_VISIT(self->_args);
  Py_VISIT(self->_callback);
  Py_VISIT(self->_source_traceback);
  Py_VISIT(self->_context);
  Py_VISIT(self->_loop);
  Py_VISIT(self->_repr);
  return 0;
}

static int _timerhandle_init(TimerHandleObject *self, PyObject *when,
                             PyObject *callback, PyObject *args, PyObject *loop,
                             PyObject *context) {
  if (when == NULL || Py_IsNone(when)) {
    PyErr_SetNone(PyExc_AssertionError);
    return -1;
  }
  if (_handle_init(self, context, args, loop, callback) < 0) return -1;
  HANDLE_XSETATTR(self->_when, when);
  self->_scheduled = 0;
  return 0;
}

Py_hash_t TimerHandle_Hash(TimerHandleObject *self) {
  return PyObject_Hash(self->_when);
}

static int _timerhandle_cancel(TimerHandleObject *self) {
  if (!self->_cancelled) {
    if (Loop__timer_handle_cancelled(self->_loop, (PyObject *)self) < 0) {
      return -1;
    }
  }
  /* super().cancel() under the hood */
  return _handle_cancel((HandleObject *)self);
}

static inline PyObject *_timehandle_when(TimerHandleObject *self) {
  return self->_when;
}

static inline PyObject *timerhandle_rich_comapre(TimerHandleObject *self,
                                                 PyObject *other, int i) {
  if (Py_IS_TYPE(other, &TimerHandle_Type) ||
      PyObject_TypeCheck(other, &TimerHandle_Type)) {
    switch (i) {
      case Py_LE: {
        TimerHandleObject *h = (TimerHandleObject *)other;
        return PyBool_FromLong(
            (PyObject_RichCompareBool(self->_when, h->_when, Py_LT) ||
             (PyObject_RichCompareBool(self->_when, h->_when, Py_EQ) &&
              PyObject_RichCompareBool(self->_callback, h->_callback, Py_EQ) &&
              PyObject_RichCompareBool(self->_args, h->_args, Py_EQ) &&
              PyObject_RichCompareBool(self->_cancelled, h->_cancelled,
                                       Py_EQ))));
      }
      case Py_GE: {
        TimerHandleObject *h = (TimerHandleObject *)other;
        return PyBool_FromLong(
            (PyObject_RichCompareBool(self->_when, h->_when, Py_GT) ||
             (PyObject_RichCompareBool(self->_when, h->_when, Py_EQ) &&
              PyObject_RichCompareBool(self->_callback, h->_callback, Py_EQ) &&
              PyObject_RichCompareBool(self->_args, h->_args, Py_EQ) &&
              PyObject_RichCompareBool(self->_cancelled, h->_cancelled,
                                       Py_EQ))));
      }
      case Py_EQ: {
        TimerHandleObject *h = (TimerHandleObject *)other;

        return PyBool_FromLong(
            (PyObject_RichCompareBool(self->_when, h->_when, Py_EQ) &&
             PyObject_RichCompareBool(self->_callback, h->_callback, Py_EQ) &&
             PyObject_RichCompareBool(self->_args, h->_args, Py_EQ) &&
             PyObject_RichCompareBool(self->_cancelled, h->_cancelled, Py_EQ)));
      }
      default:
        return PyObject_RichCompare(self->_when,
                                    ((TimerHandleObject *)other)->_when, i);
    }
  }
  PyErr_SetNone(PyExc_NotImplementedError);
  return NULL;
}
