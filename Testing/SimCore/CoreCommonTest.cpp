/* -*- mode: c++ -*- */
/****************************************************************************
 *****                                                                  *****
 *****                   Classification: UNCLASSIFIED                   *****
 *****                    Classified By:                                *****
 *****                    Declassify On:                                *****
 *****                                                                  *****
 ****************************************************************************
 *
 *
 * Developed by: Naval Research Laboratory, Tactical Electronic Warfare Div.
 *               EW Modeling & Simulation, Code 5773
 *               4555 Overlook Ave.
 *               Washington, D.C. 20375-5339
 *
 * License for source code at https://simdis.nrl.navy.mil/License.aspx
 *
 * The U.S. Government retains all rights to use, duplicate, distribute,
 * disclose, or release this software.
 *
 */
#include <cstdio>
#include <stdexcept>
#include "simCore.h"

namespace
{

int testFailure()
{
  int rv = 0;
  rv += SDK_ASSERT(rv == 0);
  rv += SDK_ASSERT(SDK_ASSERT(rv == 0) == 0);
  // Note: Expected test failure text printed to output on this failure
  rv += SDK_ASSERT(SDK_ASSERT(rv == 1) != 0);
  return rv;
}

int testVersion()
{
  int rv = 0;
  rv += SDK_ASSERT(simCore::majorVersion() == simCore::SDKVERSION_MAJOR);
  rv += SDK_ASSERT(simCore::minorVersion() == simCore::SDKVERSION_MINOR);
  rv += SDK_ASSERT(simCore::revisionVersion() == simCore::SDKVERSION_REVISION);
  rv += SDK_ASSERT(simCore::buildNumber() == simCore::SDKVERSION_BUILDNUMBER);
  // Form the build string in a different manner from the typical code to ensure it matches expectations
  // Uses C - stdio on purpose to provide different formatting path to validate answer
  char versionString[256];
  // sprintf returns number of characters printed, should be >= 5 due to formatting string
  rv += SDK_ASSERT(5 <= sprintf(versionString, "%d.%d.%d", simCore::majorVersion(), simCore::minorVersion(), simCore::revisionVersion()));
  rv += SDK_ASSERT(simCore::versionString() == versionString);
  return rv;
}

int testException()
{
  int rv = 0;
  // Create 3 classes of exceptions: simCore one, STD one, and Unknown one
  SIMCORE_EXCEPTION(simCoreException);
  class StdException : public std::logic_error
  {
  public:
    explicit StdException(const std::string& what) : std::logic_error(what) {}
  };
  class UnknownException
  {
  public:
    UnknownException() {}
  };
  // Throw each exception using the SAFETRYBEGIN/SAFETRYEND syntax (use SAFETRYCATCH to macro this)
  SAFETRYCATCH(throw SIMCORE_MAKE_EXCEPTION(simCoreException, "Purposefully thrown"), "and successfully caught");
  SAFETRYCATCH(throw StdException("Purposefully thrown"), "and successfully caught");
  SAFETRYCATCH(throw UnknownException(), "and successfully caught");

  // Test various features of the new exception class
  simCoreException ex("File.cpp", "Reason", 100);
  std::string what = ex.what();
  rv += SDK_ASSERT(what.find("Reason") != std::string::npos);
  rv += SDK_ASSERT(what.find("at line 100") != std::string::npos);
  rv += SDK_ASSERT(what.find("File.cpp") != std::string::npos);
  rv += SDK_ASSERT(ex.rawWhat() == "Reason");
  rv += SDK_ASSERT(ex.line() == 100);

  return rv;
}

}

int CoreCommonTest(int argc, char* arv[])
{
  int rv = 0;
  rv += SDK_ASSERT(testFailure() == 0);
  rv += SDK_ASSERT(testVersion() == 0);
  rv += SDK_ASSERT(testException() == 0);
  return rv;
}

