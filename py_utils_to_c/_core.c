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


// SNOWLFLAKE LIST IMPLEMENTATIONS

static inline Py_ssize_t
_dpy_utils_snowflakelist_search(PyObject *snowflake_list , PyObject *target_obj, const int is_add)
{
    const unsigned long long target = PyLong_AsUnsignedLongLong(target_obj);
    if (target == (unsigned long long)-1 && PyErr_Occurred()) {
        return -1;
    }

    Py_buffer view;
    if (PyObject_GetBuffer(snowflake_list, &view, PyBUF_SIMPLE) < 0) {
        return -1;
    }

    const uint64_t *array = (const uint64_t *)view.buf;
    Py_ssize_t low = 0;
    Py_ssize_t high = view.len / sizeof(uint64_t);

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
dpy_utils_snowflakelist_has(PyObject *self, PyObject *args)
{
    const Py_ssize_t nargs = PyTuple_Size(args);
    if (nargs != 2) {
        PyErr_Format(PyExc_TypeError, "expected 2 arguments, got %zd instead", nargs);
        return NULL;
    }

    PyObject *snowflake_list = PyTuple_GET_ITEM(args, 0);
    if (snowflake_list == NULL) {
        return NULL;
    }

    PyObject *target = PyTuple_GET_ITEM(args, 1);
    if (target == NULL) {
        return NULL;
    }
    
    if (_dpy_utils_snowflakelist_search(snowflake_list, target, 0) == -1) {
        Py_RETURN_FALSE;
    }

    Py_RETURN_TRUE;
}


static PyObject *
dpy_utils_snowflakelist_get(PyObject *self, PyObject *args)
{
    const Py_ssize_t nargs = PyTuple_Size(args);
    if (nargs != 2) {
        PyErr_Format(PyExc_TypeError, "expected 2 arguments, got %zd instead", nargs);
        return NULL;
    }

    PyObject *snowflake_list = PyTuple_GET_ITEM(args, 0);
    if (snowflake_list == NULL) {
        return NULL;
    }

    PyObject *target = PyTuple_GET_ITEM(args, 1);
    if (target == NULL) {
        return NULL;
    }

    const Py_ssize_t index = _dpy_utils_snowflakelist_search(snowflake_list, target, 0);
    if (index == -1) {
        Py_RETURN_NONE;
    }
    
    return PyLong_FromSsize_t(index);
}


static PyObject *
dpy_utils_snowflakelist_add(PyObject *self, PyObject *args)
{
    const Py_ssize_t nargs = PyTuple_Size(args);
    if (nargs != 2) {
        PyErr_Format(PyExc_TypeError, "expected 2 arguments, got %zd instead", nargs);
        return NULL;
    }

    PyObject *snowflake_list = PyTuple_GET_ITEM(args, 0);
    if (snowflake_list == NULL) {
        return NULL;
    }

    PyObject *target = PyTuple_GET_ITEM(args, 1);
    if (target == NULL) {
        return NULL;
    }
    
    const Py_ssize_t index = _dpy_utils_snowflakelist_search(snowflake_list, target, 1);
    if (index == -1) {
        return NULL;
    }

    PyObject *result = PyObject_CallMethod(snowflake_list, "insert", "nO", index, target);
    if (result == NULL) {
        return NULL;
    }

    Py_DECREF(result);
    return result;
}


static PyMethodDef methods[] = {
    {"dpy_utils_get", (PyCFunction)dpy_utils_get, METH_VARARGS | METH_KEYWORDS, "C like impl of discord.py.utils._get"},
    {"dpy_utils_find", (PyCFunction)dpy_utils_find, METH_VARARGS, "C like impl of discord.py.utils._find"},

    {"dpy_utils_snowflakelist_has", (PyCFunction)dpy_utils_snowflakelist_has, METH_VARARGS, "C like impl of discord.py.utils.SnowflakeList.has"},
    {"dpy_utils_snowflakelist_get", (PyCFunction)dpy_utils_snowflakelist_get, METH_VARARGS, "C like impl of discord.py.utils.SnowflakeList.get"},
    {"dpy_utils_snowflakelist_add", (PyCFunction)dpy_utils_snowflakelist_add, METH_VARARGS, "C like impl of discord.py.utils.SnowflakeList.add"},
    {NULL, NULL, 0, NULL}
};

static struct PyModuleDef module = {
    PyModuleDef_HEAD_INIT,
    "py_utils_to_c",
    NULL,
    -1,
    methods
};

PyMODINIT_FUNC PyInit_py_utils_to_c(void)
{
    return PyModule_Create(&module);
}