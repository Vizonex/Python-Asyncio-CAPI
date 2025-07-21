# cython: freethreading_compatible = True
from cpython.object cimport PyObject as PyObject


cdef extern from "casyncio.h":
    """
/* ========================= IMPORTANT ========================= */
/*     Module This was Externed from casyncio.__init__.pxd       */
    """
    # Returns -1 on fail. required you import this first otherwise
    # expect segfaults...
    int import_casyncio "Import_CAsyncio"() 

    # Structures are too dynamic to define in cython so I wrote a C-API for gaining the important parts.
    struct FutureObj:
        pass

    ctypedef class _asyncio.Future [struct FutureObj, check_size ignore]:
        pass
    
    # Cython already does some strict checks on its own so we didn't
    # need any internal checks. This way we save a bit of time. 
    # And time is also saved during execution
    bint FutureObj_IsAlive(Future fut)
    int FutureObj_EnsureAlive(Future fut) except -1
    

    # NOTE: You might be wondering why I didn't include this function
    # FutureObj_ScheduleCallbacks(FutureObj* fut)
    # It's internal and is not really made to be used by average Cython / CPython Developers
    # however it will invoke when You set a result or exception or Cancel a future

    int FutureObj_SetResult(Future fut, object res) except -1
    int FutureObj_SetException(Future fut, object exc) except -1
    void FutureObj_SetCancelledError(Future fut)
    int FutureObj_GetResult(Future fut, PyObject** result) except -1
    int FutureObj_AddDoneCallback(Future fut, object func, object ctx) except -1



