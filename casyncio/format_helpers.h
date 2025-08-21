#ifndef __FORMAT_HELPERS_H__
#define __FORMAT_HELPERS_H__

#include <Python.h>
#include "pythoncapi_compat.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DEBUG_STACK_DEPTH 10 /* should be 10 as of 3.10 throw an issue on github if this changes on future versions */



/* Implementation is only for being inline with StackSummary which is a subclass of list to only a certain extent,
this also takes frame_gen into account for us */


// PyObject* extract_stack_impl(PyFrameObject* f, PyObject* limit){
//     if (f == NULL || Py_IsNone((PyObject*)f)){
//         f = (PyFrameObject*)Py_NewRef(PyEval_GetFrame()->f_back);
//     }
//     if (limit == NULL || Py_IsNone(limit)){
//         StackSummary_Extract_Impl();
//     }
// }

/* Will need to make a module for this but for now use globals */
PyObject* functools_partial;
PyObject* functools_partialmethod;
/*

def _format_args_and_kwargs(args, kwargs):
    """Format function arguments and keyword arguments.

    Special case for a single parameter: ('hello',) is formatted as ('hello').
    """
    # use reprlib to limit the length of the output
    items = []
    if args:
        items.extend(reprlib.repr(arg) for arg in args)
    if kwargs:
        items.extend(f'{k}={reprlib.repr(v)}' for k, v in kwargs.items())
    return '({})'.format(', '.join(items))

*/

PyObject* _format_args_and_kwargs(PyObject* args, PyObject *kwargs){
    PyUnicodeWriter* writer = PyUnicodeWriter_Create(1024);
    if (writer == NULL) return NULL;

    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    /* as a bonus feature I also let NULL slide... */
    Py_ssize_t nkwargs = (kwargs != NULL || Py_IsNone(kwargs)) ? PyDict_GET_SIZE(kwargs) : 0;

    if (PyUnicodeWriter_WriteChar(writer, '(') < 0) 
        goto fail;


    /* For now use PyObject_Repr in the future use reprlib.repr(...) */
    Py_ssize_t endpos = nargs - 1;
    if (nargs > 0){
        for (Py_ssize_t i = 0; i < nargs; i++){
            if (PyUnicodeWriter_WriteRepr(writer, PyTuple_GET_ITEM(args, i)) < 0){
                goto fail;
            }
            if (i != endpos){
                if (PyUnicodeWriter_WriteChar(writer, ',') < 0) 
                    goto fail;
                if (PyUnicodeWriter_WriteChar(writer, ' ') < 0) 
                    goto fail;
            }
        }
    }
    if (nkwargs > 0){
        endpos = nkwargs - 1;
        Py_ssize_t pos = 0;
        PyObject* k = NULL, *v = NULL;
        while (PyDict_Next(kwargs, &pos, &k, &v)){
            if (PyUnicodeWriter_WriteStr(writer, k) < 0){
                goto fail;
            }
            if (PyUnicodeWriter_WriteChar(writer, '=') < 0) 
                goto fail;

            if (PyUnicodeWriter_WriteRepr(writer, k) < 0){
                goto fail;
            }
            if (pos != endpos){
                if (PyUnicodeWriter_WriteChar(writer, ',') < 0) 
                    goto fail;
                if (PyUnicodeWriter_WriteChar(writer, ' ') < 0) 
                    goto fail;
            }
        }
    }
    return PyUnicodeWriter_Finish(writer);

    fail:
        PyUnicodeWriter_Discard(writer);
        return NULL;
}



PyObject* _format_callback(PyObject* func, PyObject* args, PyObject* kwargs, PyObject* suffix){

    if (PyObject_IsInstance(func, functools_partial)){
        PyObject* _suffix = _format_args_and_kwargs(args, kwargs);
        if (_suffix == NULL) return NULL;
        if (suffix != NULL){
            PyUnicode_AppendAndDel(&_suffix, suffix);
        }
        /* recurse again */
        return _format_callback(
            PyObject_GetAttrString(func, "func"),
            PyObject_GetAttrString(func, "args"),
            PyObject_GetAttrString(func, "kwargs"),
            _suffix
        );
    }
    PyObject* func_repr = PyObject_GetAttrString(func, "__qualname__");
    if (func_repr == NULL){
        func_repr = PyObject_GetAttrString(func, "__name__");
        if (func_repr == NULL){
            func_repr = PyObject_Repr(func);
            if (func_repr == NULL){
                /* FAIL */
                Py_XDECREF(suffix);
                return NULL;
            }
        }
    }
    if (suffix != NULL){
        PyUnicode_AppendAndDel(&func_repr, suffix);
    };
    return func_repr;
}

PyObject* inspect_unwrap_impl(PyObject* func){
    PyObject* f = func;
    Py_ssize_t rec = 0;
    Py_ssize_t limit = Py_GetRecursionLimit();
    PyObject* __wrapped__ = PyUnicode_FromString("__wrapped__");
    PyObject* memo = PySet_New(PyList_New(0));
    PyObject* w_func;
    while (1) {
        w_func = PyObject_GetAttr(f, __wrapped__);
        if (w_func == NULL){
            /* function Obtained */
            break;
        }
        /* Same as calling id(...) but without auditing */
        PyObject* func_id = PyLong_FromVoidPtr(f);

        if ((rec > limit) || PySet_Contains(memo, func_id)){
            PyErr_Format(PyExc_ValueError, "wrapper loop when unwrapping %R", func);
            Py_CLEAR(memo);
            return NULL;
        }
        f = w_func;
        if (PySet_Add(memo, func_id) < 0){
            Py_CLEAR(memo);
            return NULL;
        };
        rec++;
    }
    Py_CLEAR(memo);
    return f;
}

int _get_function_source(PyObject* func, PyObject** filename, PyObject** firstlineno){
    PyObject* _func = inspect_unwrap_impl(func);
    if (PyFunction_Check(_func)){
        PyCodeObject* code = (PyCodeObject*)PyFunction_GET_CODE(_func);
        *filename = code->co_filename;
        *firstlineno = code->co_firstlineno;
        return 1;
    } 
    if (PyObject_IsInstance(_func, functools_partial) || PyObject_IsInstance(_func, functools_partialmethod)){
        return _get_function_source(PyObject_GetAttrString(_func, "func"), filename, firstlineno);
    }
    return 0;
}

PyObject* _format_callback_source(PyObject* func, PyObject* args){
    PyObject* func_repr = _format_callback(func, args, NULL, NULL);
    PyObject* filename = NULL, *lineno = NULL;
    if (_get_function_source(func, filename, lineno)){
        PyUnicode_AppendAndDel(func_repr, PyUnicode_FromFormat(" at %R:%R", filename, lineno));
    }
    return func_repr;
}


#ifdef __cplusplus
}
#endif
#endif // __FORMAT_HELPERS_H__