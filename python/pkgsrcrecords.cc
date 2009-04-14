// -*- mode: cpp; mode: fold -*-
// Description								/*{{{*/
// $Id: pkgsrcrecords.cc,v 1.2 2003/12/26 17:04:22 mdz Exp $
/* ######################################################################

   Package Records - Wrapper for the package records functions

   ##################################################################### */
									/*}}}*/
// Include Files							/*{{{*/
#include "generic.h"
#include "apt_pkgmodule.h"

#include <apt-pkg/sourcelist.h>

#include <Python.h>
									/*}}}*/

struct PkgSrcRecordsStruct
{
   pkgSourceList List;
   pkgSrcRecords *Records;
   pkgSrcRecords::Parser *Last;

   PkgSrcRecordsStruct() : Last(0) {
      List.ReadMainList();
      Records = new pkgSrcRecords(List);
   };
   ~PkgSrcRecordsStruct() {
      delete Records;
   };
};

// PkgSrcRecords Class							/*{{{*/
// ---------------------------------------------------------------------

static char *doc_PkgSrcRecordsLookup = "xxx";
static PyObject *PkgSrcRecordsLookup(PyObject *Self,PyObject *Args)
{
   PkgSrcRecordsStruct &Struct = GetCpp<PkgSrcRecordsStruct>(Self);

   char *Name = 0;
   if (PyArg_ParseTuple(Args,"s",&Name) == 0)
      return 0;

   Struct.Last = Struct.Records->Find(Name, false);
   if (Struct.Last == 0) {
      Struct.Records->Restart();
      Py_INCREF(Py_None);
      return HandleErrors(Py_None);
   }

   return Py_BuildValue("i", 1);
}

static char *doc_PkgSrcRecordsRestart = "Start Lookup from the begining";
static PyObject *PkgSrcRecordsRestart(PyObject *Self,PyObject *Args)
{
   PkgSrcRecordsStruct &Struct = GetCpp<PkgSrcRecordsStruct>(Self);

   char *Name = 0;
   if (PyArg_ParseTuple(Args,"") == 0)
      return 0;

   Struct.Records->Restart();

   Py_INCREF(Py_None);
   return HandleErrors(Py_None);
}

static PyMethodDef PkgSrcRecordsMethods[] =
{
   {"Lookup",PkgSrcRecordsLookup,METH_VARARGS,doc_PkgSrcRecordsLookup},
   {"Restart",PkgSrcRecordsRestart,METH_VARARGS,doc_PkgSrcRecordsRestart},
   {}
};

/**
 * Get the PkgSrcRecordsStruct from a PyObject. If no package has been looked
 * up, set an AttributeError using the given name.
 */
static inline PkgSrcRecordsStruct &GetStruct(PyObject *Self,char *name) {
   PkgSrcRecordsStruct &Struct = GetCpp<PkgSrcRecordsStruct>(Self);
   if (Struct.Last == 0)
      PyErr_SetString(PyExc_AttributeError,name);
   return Struct;
}

static PyObject *PkgSrcRecordsGetPackage(PyObject *Self,void*) {
   PkgSrcRecordsStruct &Struct = GetStruct(Self,"Package");
   return (Struct.Last != 0) ? CppPyString(Struct.Last->Package()) : 0;
}
static PyObject *PkgSrcRecordsGetVersion(PyObject *Self,void*) {
   PkgSrcRecordsStruct &Struct = GetStruct(Self,"Version");
   return (Struct.Last != 0) ? CppPyString(Struct.Last->Version()) : 0;
}
static PyObject *PkgSrcRecordsGetMaintainer(PyObject *Self,void*) {
   PkgSrcRecordsStruct &Struct = GetStruct(Self,"Maintainer");
   return (Struct.Last != 0) ? CppPyString(Struct.Last->Maintainer()) : 0;
}
static PyObject *PkgSrcRecordsGetSection(PyObject *Self,void*) {
   PkgSrcRecordsStruct &Struct = GetStruct(Self,"Section");
   return (Struct.Last != 0) ? CppPyString(Struct.Last->Section()) : 0;
}
static PyObject *PkgSrcRecordsGetRecord(PyObject *Self,void*) {
   PkgSrcRecordsStruct &Struct = GetStruct(Self,"Record");
   return (Struct.Last != 0) ? CppPyString(Struct.Last->AsStr()) : 0;
}
static PyObject *PkgSrcRecordsGetBinaries(PyObject *Self,void*) {
   PkgSrcRecordsStruct &Struct = GetStruct(Self,"Binaries");
   if (Struct.Last == 0)
      return 0;
   PyObject *List = PyList_New(0);
   for(const char **b = Struct.Last->Binaries(); *b != 0; ++b)
      PyList_Append(List, CppPyString(*b));
   return List; // todo
}
static PyObject *PkgSrcRecordsGetIndex(PyObject *Self,void*) {
   PkgSrcRecordsStruct &Struct = GetStruct(Self,"Index");
   if (Struct.Last == 0)
      return 0;
   const pkgIndexFile &tmp = Struct.Last->Index();
   return CppOwnedPyObject_NEW<pkgIndexFile*>(Self,&PackageIndexFileType,
                                              (pkgIndexFile*)&tmp);
}

static PyObject *PkgSrcRecordsGetFiles(PyObject *Self,void*) {
   PkgSrcRecordsStruct &Struct = GetStruct(Self,"Files");
   if (Struct.Last == 0)
      return 0;
   PyObject *List = PyList_New(0);

   vector<pkgSrcRecords::File> f;
   if(!Struct.Last->Files(f))
      return NULL; // error

   PyObject *v;
   for(unsigned int i=0;i<f.size();i++) {
      v = Py_BuildValue("(siss)",
			f[i].MD5Hash.c_str(),
			f[i].Size,
			f[i].Path.c_str(),
			f[i].Type.c_str());
      PyList_Append(List, v);
      Py_DECREF(v);
   }
   return List;
}

static PyObject *PkgSrcRecordsGetBuildDepends(PyObject *Self,void*) {
   PkgSrcRecordsStruct &Struct = GetStruct(Self,"BuildDepends");
   if (Struct.Last == 0)
      return 0;
   PyObject *List = PyList_New(0);

   vector<pkgSrcRecords::Parser::BuildDepRec> bd;
   if(!Struct.Last->BuildDepends(bd, false /* arch-only*/))
      return NULL; // error

   PyObject *v;
   for(unsigned int i=0;i<bd.size();i++) {
      v = Py_BuildValue("(ssii)", bd[i].Package.c_str(),
			bd[i].Version.c_str(), bd[i].Op, bd[i].Type);
      PyList_Append(List, v);
      Py_DECREF(v);
   }
   return List;
}

static PyGetSetDef PkgSrcRecordsGetSet[] = {
   {"Binaries",PkgSrcRecordsGetBinaries},
   {"BuildDepends",PkgSrcRecordsGetBuildDepends},
   {"Files",PkgSrcRecordsGetFiles},
   {"Index",PkgSrcRecordsGetIndex},
   {"Maintainer",PkgSrcRecordsGetMaintainer},
   {"Package",PkgSrcRecordsGetPackage},
   {"Record",PkgSrcRecordsGetRecord},
   {"Section",PkgSrcRecordsGetSection},
   {"Version",PkgSrcRecordsGetVersion},
   {}
};

PyTypeObject PkgSrcRecordsType =
{
   PyObject_HEAD_INIT(&PyType_Type)
   0,			                // ob_size
   "pkgSrcRecords",                          // tp_name
   sizeof(CppPyObject<PkgSrcRecordsStruct>),   // tp_basicsize
   0,                                   // tp_itemsize
   // Methods
   CppDealloc<PkgSrcRecordsStruct>,   // tp_dealloc
   0,                                   // tp_print
   0,                                   // tp_getattr
   0,                                   // tp_setattr
   0,                                   // tp_compare
   0,                                   // tp_repr
   0,                                   // tp_as_number
   0,                                   // tp_as_sequence
   0,                                   // tp_as_mapping
   0,                                   // tp_hash
   0,                                   // tp_call
   0,                                   // tp_str
   0,                                   // tp_getattro
   0,                                   // tp_setattro
   0,                                   // tp_as_buffer
   Py_TPFLAGS_DEFAULT,                  // tp_flags
   "SourceRecords Object",              // tp_doc
   0,                                   // tp_traverse
   0,                                   // tp_clear
   0,                                   // tp_richcompare
   0,                                   // tp_weaklistoffset
   0,                                   // tp_iter
   0,                                   // tp_iternext
   PkgSrcRecordsMethods,                   // tp_methods
   0,                                   // tp_members
   PkgSrcRecordsGetSet,                    // tp_getset
};

									/*}}}*/

PyObject *GetPkgSrcRecords(PyObject *Self,PyObject *Args)
{
#if 0
   PyObject *Owner;
   if (PyArg_ParseTuple(Args,"O!",&PkgCacheType,&Owner) == 0)
      return 0;

   return HandleErrors(CppOwnedPyObject_NEW<PkgSrcRecordsStruct>(Owner,
			   &PkgSrcRecordsType));
#endif
   if (PyArg_ParseTuple(Args,"") == 0)
      return 0;

   return HandleErrors(CppPyObject_NEW<PkgSrcRecordsStruct>(&PkgSrcRecordsType));
}

