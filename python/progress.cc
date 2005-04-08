// Description								/*{{{*/
// $Id: progress.cc,v 1.5 2003/06/03 03:03:23 mdz Exp $
/* ######################################################################

   Progress - Wrapper for the progress related functions

   ##################################################################### */

#include <iostream>
#include <sys/types.h>
#include <sys/wait.h>
#include "progress.h"

// generic
bool PyCallbackObj::RunSimpleCallback(const char* method_name, 
				      PyObject *arglist)
{
   if(callbackInst == 0)
      return false;

   PyObject *method = PyObject_GetAttrString(callbackInst,(char*) method_name);
   if(method == NULL) {
      // FIXME: make this silent
      std::cerr << "Can't find '" << method_name << "' method" << std::endl;
      Py_XDECREF(arglist);
      return false;
   }
   PyObject *result = PyEval_CallObject(method, arglist);
   Py_XDECREF(arglist);

   if(result == NULL) {
      // exception happend
      std::cerr << "Error in function " << method_name << std::endl;
      PyErr_Print();

      return NULL;
   }

   Py_XDECREF(result);
   Py_XDECREF(method);

   return true;
}


// OpProgress interface 
// FIXME: add "string Op, string SubOp" as attribute to the callbackInst
void PyOpProgress::Update() 
{
   // Build up the argument list... 
   PyObject *arglist = Py_BuildValue("(f)", Percent);
   RunSimpleCallback("Update", arglist);
};

void PyOpProgress::Done()
{
   RunSimpleCallback("Done");
}



// fetcher interface

enum {
   DLDone, DLQueued, DLFailed, DLHit
};


// apt interface
bool PyFetchProgress::MediaChange(string Media, string Drive)
{
   //std::cout << "MediaChange" << std::endl;
   PyObject *arglist = Py_BuildValue("(ss)", Media.c_str(), Drive.c_str());
   RunSimpleCallback("MediaChange", arglist);
   
   // FIXME: need to return depending on the python result
   return true;
}

void PyFetchProgress::UpdateStatus(pkgAcquire::ItemDesc &Itm, int status)
{
   //std::cout << "UpdateStatus: " << Itm.URI << " " << status << std::endl;
   PyObject *arglist = Py_BuildValue("(sssi)", Itm.URI.c_str(), Itm.Description.c_str(), Itm.ShortDesc.c_str(), status);
   RunSimpleCallback("UpdateStatus", arglist);
}

void PyFetchProgress::IMSHit(pkgAcquire::ItemDesc &Itm)
{
   UpdateStatus(Itm, DLHit);
}

void PyFetchProgress::Fetch(pkgAcquire::ItemDesc &Itm)
{
   UpdateStatus(Itm, DLQueued);
}

void PyFetchProgress::Done(pkgAcquire::ItemDesc &Itm)
{
   UpdateStatus(Itm, DLDone);
}

void PyFetchProgress::Fail(pkgAcquire::ItemDesc &Itm)
{
   UpdateStatus(Itm, DLFailed);
}

void PyFetchProgress::Start()
{
   //std::cout << "Start" << std::endl;
   pkgAcquireStatus::Start();
   RunSimpleCallback("Start");
}


void PyFetchProgress::Stop()
{
   //std::cout << "Stop" << std::endl;
   pkgAcquireStatus::Stop();
   RunSimpleCallback("Stop");
}

// FIXME: it should just set the attribute for
//         CurrentCPS, Current...
bool PyFetchProgress::Pulse(pkgAcquire * Owner)
{
   pkgAcquireStatus::Pulse(Owner);

   //std::cout << "Pulse" << std::endl;
   if(callbackInst == 0)
      return false;
   
   // set stats
   PyObject *o;
   o = Py_BuildValue("f", CurrentCPS);
   PyObject_SetAttrString(callbackInst, "CurrentCPS", o);
   o = Py_BuildValue("f", CurrentBytes);
   PyObject_SetAttrString(callbackInst, "CurrentBytes", o);
   o = Py_BuildValue("i", CurrentItems);
   PyObject_SetAttrString(callbackInst, "CurrentItems", o);
   o = Py_BuildValue("i", TotalItems);
   PyObject_SetAttrString(callbackInst, "TotalItems", o);
   o = Py_BuildValue("f", TotalBytes);
   PyObject_SetAttrString(callbackInst, "TotalBytes", o);
   
   RunSimpleCallback("Pulse");

   
   // this can be canceld by returning false
   return true;
}



// install progress

void PyInstallProgress::StartUpdate() 
{
   RunSimpleCallback("StartUpdate");
}

void PyInstallProgress::UpdateInterface() 
{
   RunSimpleCallback("UpdateInterface");
}
 
void PyInstallProgress::FinishUpdate() 
{
   RunSimpleCallback("FinishUpdate");
}

pkgPackageManager::OrderResult PyInstallProgress::Run(pkgPackageManager *pm) 
{
   void *dummy;
   pkgPackageManager::OrderResult res;
   int ret;
   pid_t child_id;

#if 0 // FIXME: this needs to be merged into apt to support medium swaping
   res = pm->DoInstallPreFork();
   if (res == pkgPackageManager::Failed)
       return res;
#endif

   // support custom fork methods
   if(PyObject_HasAttrString(callbackInst, "fork")) {
      PyObject *method = PyObject_GetAttrString(callbackInst, "fork");
      //std::cerr << "custom fork found" << std::endl;
      PyObject *arglist = Py_BuildValue("()");
      PyObject *result = PyEval_CallObject(method, arglist);
      Py_DECREF(arglist);       
      if (result == NULL) {
	 std::cerr << "fork method invalid" << std::endl;
	 PyErr_Print();
	 return pkgPackageManager::Failed;
      }
      if(!PyArg_Parse(result, "i", &child_id) )
	 std::cerr << "result could not be parsed?"<< std::endl;
      //std::cerr << "got: " << child_id << std::endl;
   } else {
      //std::cerr << "using build-in fork()" << std::endl;
      child_id = fork();
   }
   

#if 0 // FIXME: this needs to be merged into apt to support medium swaping
   if (child_id == 0) {
      res = pm->DoInstallPostFork();
      _exit(res);
   }
#endif
   if (child_id == 0) {
      res = pm->DoInstall();
      _exit(res);
   }

   StartUpdate();
   while (waitpid(child_id, &ret, WNOHANG) == 0)
      UpdateInterface();

   res = (pkgPackageManager::OrderResult) WEXITSTATUS(ret);

   FinishUpdate();

   return res;
}