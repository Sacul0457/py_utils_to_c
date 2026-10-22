#define PY_SSIZE_T_CLEAN
#include <Python.h>

// Core implementation of the utility in C


static inline PyObject *
multi_getattr(PyObject *obj, PyObject *attrs)
{
    assert(obj != NULL && attrs != NULL);

    Py_INCREF(obj);
    const Py_ssize_t len = PySequence_Length(attrs);
    if (len == -1) {
        return NULL;
    }
    for (Py_ssize_t i = 0; i < len; i++) {
        PyObject *next = PyObject_GetAttr(obj, PyList_GET_ITEM(attrs, i));
        Py_DECREF(obj);
        if (next == NULL) {
           return NULL; 
        }
        obj = next;
    }
    return obj;
}

static PyObject *
dpy_utils_get(PyObject *self, PyObject *args, PyObject *kwargs)
{
    const Py_ssize_t nargs = PyTuple_Size(args);
    if (nargs != 1) {
        PyErr_Format(PyExc_TypeError, "expected 1 argument, got %zd instead", nargs);
        return NULL;
    }
    
    PyObject *sequence = PyTuple_GetItem(args, 0);
    if (!PySequence_Check(sequence)) {
        PyErr_Format(PyExc_TypeError, "expected sequence, got %.200s instead", Py_TYPE(sequence)->tp_name);
        return NULL;
    }

    const Py_ssize_t len = PySequence_Length(sequence);
    if (len == -1) {
        return NULL;
    }

    const Py_ssize_t no_of_kwargs = PyMapping_Length(kwargs);
    if (no_of_kwargs == -1) {
        return NULL;
    }
    PyObject *key_values = PyTuple_New(no_of_kwargs * 2); // (('attr_0', 'attr_1'), value_0, ('attr_2', 'attr_3'), value_1, ...)
    if (key_values == NULL) {
        PyErr_SetString(PyExc_MemoryError, "PyTuple_New returned NULL");
        return NULL;
    }

    PyObject *key;
    PyObject *value;
    Py_ssize_t pos = 0;

    Py_ssize_t index = 0;
    PyObject *double_underscore = PyUnicode_FromString("__");
    if (double_underscore == NULL) {
        goto error;
    }
    while (PyDict_Next(kwargs, &pos, &key, &value)) {
        PyObject *attrs = PyUnicode_Split(key, double_underscore, PyUnicode_GetLength(key));
        if (attrs == NULL) {
            Py_DECREF(double_underscore);
            goto error;  
        }
        PyTuple_SET_ITEM(key_values, index++, attrs);

        Py_INCREF(value);
        PyTuple_SET_ITEM(key_values, index++, value);
    }
    Py_DECREF(double_underscore);

    PyObject *ret = Py_None;
    for (Py_ssize_t i = 0; i < len; i++) {
        PyObject *item = PySequence_Fast_GET_ITEM(sequence, i);

        for (Py_ssize_t j = 0; j < no_of_kwargs * 2; j += 2) {
            PyObject *attrs = PyTuple_GET_ITEM(key_values, j);
            PyObject *cmp_value = PyTuple_GET_ITEM(key_values, j + 1);
            

            PyObject *attr_value = multi_getattr(item, attrs);
            if (attr_value == NULL) {
                goto error;
            }
            int err = PyObject_RichCompareBool(attr_value, cmp_value, Py_EQ);
            Py_DECREF(attr_value);
            if (err == 1) {
                continue;
            }
            else if (err == 0) {
                goto no_match;
            }
            goto error;
        }

        ret = item;
        Py_INCREF(ret);
        break;

        no_match:
            (void)0;
    }

    Py_DECREF(key_values);
    return ret;

error:
    Py_DECREF(key_values);
    return NULL;
}


static PyObject *
dpy_utils_find(PyObject *self, PyObject *args)
{
    const Py_ssize_t nargs = PyTuple_Size(args);
    if (nargs != 2) {
        PyErr_Format(PyExc_TypeError, "expected 2 arguments, got %zd instead", nargs);
        return NULL;
    }

    PyObject *callable = PyTuple_GetItem(args, 0);
    if (!PyCallable_Check(callable)) {
        PyErr_Format(PyExc_TypeError, "expected callable, got %.200s instead", Py_TYPE(callable)->tp_name);
        return NULL;
    }
    
    PyObject *sequence = PyTuple_GetItem(args, 1);
    if (!PySequence_Check(sequence)) {
        PyErr_Format(PyExc_TypeError, "expected sequence, got %.200s instead", Py_TYPE(sequence)->tp_name);
        return NULL;
    }

    const Py_ssize_t len = PySequence_Length(sequence);
    if (len == -1) {
        return NULL;
    }

    for (Py_ssize_t i = 0; i < len; i++) {
        PyObject *item = PySequence_Fast_GET_ITEM(sequence, i);
        PyObject *ret = PyObject_CallOneArg(callable, item);    
        if (ret == NULL) {
            return NULL;
        }

        int err = PyObject_IsTrue(ret);
        Py_DECREF(ret);
        if (err == 1) {
            Py_INCREF(item);
            return item;
        }
        if (err == -1) {
            return NULL;
        }
    }
    Py_RETURN_NONE;
}


// Binary Search Sequence IMPLEMENTATIONS

static inline Py_ssize_t
_binary_search_seq(PyObject *snowflake_list , PyObject *target_obj, const int is_add)
{
    const long long target = PyLong_AsLongLong(target_obj);
    if (target == (long long)-1 && PyErr_Occurred()) {
        return -1;
    }

    Py_buffer view;
    if (PyObject_GetBuffer(snowflake_list, &view, PyBUF_SIMPLE) < 0) {
        return -1;
    }

    const long long *array = (const long long *)view.buf;
    Py_ssize_t low = 0;
    Py_ssize_t high = view.len / sizeof(long long);

    while (low < high) {
        const Py_ssize_t mid = low + (high - low) / 2;
        if (array[mid] < target) {
            low = mid + 1;
        }
        else {
            high = mid;
        }
    }

    PyBuffer_Release(&view);
    if (is_add || low < view.len && array[low] == target) {
        return low;
    }
    
    return -1;
}


static PyObject *
binary_search_seq_has(PyObject *self, PyObject *args)
{
    const Py_ssize_t nargs = PyTuple_Size(args);
    if (nargs != 2) {
        PyErr_Format(PyExc_TypeError, "expected 2 arguments, got %zd instead", nargs);
        return NULL;
    }

    PyObject *sequence = PyTuple_GET_ITEM(args, 0);
    if (!PySequence_Check(sequence)) {
        PyErr_Format(PyExc_TypeError, "Expected a sequence, got %.200s instead.", Py_TYPE(sequence)->tp_name);
    }

    PyObject *target = PyTuple_GET_ITEM(args, 1);
    if (!PyLong_Check(target)) {
        PyErr_Format(PyExc_TypeError, "Expected an int, got %.200s instead.", Py_TYPE(target)->tp_name);
    }
    
    if (_binary_search_seq(sequence, target, 0) == -1) {
        Py_RETURN_FALSE;
    }

    Py_RETURN_TRUE;
}


static PyObject *
binary_search_seq_get(PyObject *self, PyObject *args)
{
    const Py_ssize_t nargs = PyTuple_Size(args);
    if (nargs != 2) {
        PyErr_Format(PyExc_TypeError, "expected 2 arguments, got %zd instead", nargs);
        return NULL;
    }

    PyObject *sequence = PyTuple_GET_ITEM(args, 0);
    if (!PySequence_Check(sequence)) {
        PyErr_Format(PyExc_TypeError, "Expected a sequence, got %.200s instead.", Py_TYPE(sequence)->tp_name);
    }

    PyObject *target = PyTuple_GET_ITEM(args, 1);
    if (!PyLong_Check(target)) {
        PyErr_Format(PyExc_TypeError, "Expected an int, got %.200s instead.", Py_TYPE(target)->tp_name);
    }

    const Py_ssize_t index = _binary_search_seq(sequence, target, 0);
    if (index == -1) {
        Py_RETURN_NONE;
    }
    
    return PyLong_FromSsize_t(index);
}


static inline PyObject *
_minmax_seq_no_key(PyObject *sequence)
{
    assert(sequence != NULL && PySequence_Check(sequence));

    const Py_ssize_t seq_len = PySequence_Fast_GET_SIZE(sequence);
    if (seq_len == 0) {
        PyErr_SetString(PyExc_ValueError, "Empty sequence");
        return NULL;
    }

    PyObject *min_o = PySequence_Fast_GET_ITEM(sequence, 0);
    PyObject *max_o = min_o;

    for (Py_ssize_t i = 1; i < seq_len; i++) {
        PyObject *item  = PySequence_Fast_GET_ITEM(sequence, i);

        int min_err = PyObject_RichCompareBool(item, min_o, Py_LT);

        if (min_err == 1) {
            min_o = item;
        }
        else if (min_err == -1) {
            return NULL;
        }

        int max_err = PyObject_RichCompareBool(max_o, item, Py_LT);
        if (max_err == 1) {
            max_o = item;
        }
        else if (max_err == -1) {
            return NULL;
        }
    }
    
    PyObject *tuple = PyTuple_New(2);
    if (tuple == NULL) {
        return NULL;
    }

    Py_INCREF(min_o);
    Py_INCREF(max_o);
    PyTuple_SET_ITEM(tuple, 0, min_o);
    PyTuple_SET_ITEM(tuple, 1, max_o);
    return tuple;
}


static inline PyObject *
_minmax_iter_no_key(PyObject *iterable)
{
    assert(iterable != NULL);
    PyObject *iter = PyObject_GetIter(iterable);
    if (iter == NULL) {
        return NULL;
    }

    PyObject *item = NULL;

    int err = PyIter_NextItem(iter, &item);
    if (err == 0 || err == -1) {
        Py_DECREF(iter);
        PyErr_SetString(PyExc_ValueError, "Empty iterable");
        return NULL;
    }

    PyObject *min_o = item;
    Py_INCREF(item); // 2 references
    PyObject *max_o = item;

    while (1) {
        err = PyIter_NextItem(iter, &item);
        if (err == 0) {
            break;
        }
        else if (err == -1) {
            goto error;
        }

        int min_err = PyObject_RichCompareBool(item, min_o, Py_LT);
        if (min_err == -1) {
            Py_DECREF(item);
            goto error;
        }

        int max_err = PyObject_RichCompareBool(max_o, item, Py_LT);
        if (max_err == -1) {
            Py_DECREF(item);
            goto error;
        }

        if (min_err == 0 && max_err == 0) {
            Py_DECREF(item);
        }
        else if (min_err == 1 && max_err == 1) {
            Py_INCREF(item);
        }

        if (min_err == 1) {
            Py_DECREF(min_o);
            min_o = item;
        }
        if(max_err == 1) {
            Py_DECREF(max_o);
            max_o = item;
        }
    }
    
    PyObject *tuple = PyTuple_New(2);
    if (tuple == NULL) {
        goto error;
    }

    Py_DECREF(iter);

    PyTuple_SET_ITEM(tuple, 0, min_o);
    PyTuple_SET_ITEM(tuple, 1, max_o);
    return tuple;

error:
    Py_DECREF(min_o);
    Py_DECREF(max_o);
    Py_DECREF(iter);
    return NULL;
}


static PyObject *
minmax(PyObject *self, PyObject *args)
{
    const Py_ssize_t nargs = PyTuple_Size(args);
    if (nargs != 1) {
        PyErr_Format(PyExc_TypeError, "expected 1 argument, got %zd instead", nargs);
        return NULL;
    }

    PyObject *iterable = PyTuple_GET_ITEM(args, 0);
    if (!PySequence_Check(iterable) && !PySet_Check(iterable) && !PyDictViewSet_Check(iterable) && !PyIter_Check(iterable)) {
        PyErr_Format(PyExc_TypeError, "Expected an iterable, got %.200s instead.", Py_TYPE(iterable)->tp_name);
        return NULL;
    }

    if (PySequence_Check(iterable)) {
        return _minmax_seq_no_key(iterable);
    }
    else {
        return _minmax_iter_no_key(iterable);
    }
}


static PyMethodDef methods[] = {
    {"dpy_utils_get", (PyCFunction)dpy_utils_get, METH_VARARGS | METH_KEYWORDS, "C like impl of discord.py.utils._get"},
    {"dpy_utils_find", (PyCFunction)dpy_utils_find, METH_VARARGS, "C like impl of discord.py.utils._find"},

    {"binary_search_seq_get", (PyCFunction)binary_search_seq_get, METH_VARARGS, "C like impl of a binary search on an integer sequence"},
    {"binary_search_seq_has", (PyCFunction)binary_search_seq_has, METH_VARARGS, "C like impl of a binary search on an integer sequence"},

    {"minmax", (PyCFunction)minmax, METH_VARARGS, "Returns a tuple of the (min, max) of an iterable"},
    {NULL, NULL, 0, NULL}
};

static struct PyModuleDef module = {
    PyModuleDef_HEAD_INIT,
    "py_utils_to_c",
    NULL,
    -1,
    methods
};

PyMODINIT_FUNC PyInit__core(void)
{
    return PyModule_Create(&module);
}