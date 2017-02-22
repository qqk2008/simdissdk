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
#include <cmath>
#include <string>
#include "simCore/Common/SDKAssert.h"
#include "simCore/Common/Version.h"
#include "simCore/Calc/Math.h"
#include "simCore/Calc/Random.h"
#include "simCore/Calc/Angle.h"
#include "simCore/String/Angle.h"

namespace {

int testOne(const std::string& token, double degreeVal)
{
  int rv = 0;

  double angle;
  rv += SDK_ASSERT(simCore::getAngleFromDegreeString(token, false, angle) == 0);
  rv += SDK_ASSERT(simCore::areAnglesEqual(degreeVal, angle));

  rv += SDK_ASSERT(simCore::getAngleFromDegreeString(token, true, angle) == 0);
  rv += SDK_ASSERT(simCore::areAnglesEqual(degreeVal * simCore::DEG2RAD, angle));

  return rv;
}

int testCombinations(const std::string& token, double degreeVal)
{
  int rv = 0;

  rv += testOne(token, degreeVal);
  rv += testOne(" " + token, degreeVal);
  rv += testOne(" " + token + " ", degreeVal);
  rv += testOne("-" + token, -degreeVal);
  rv += testOne(" -" + token, -degreeVal);
  rv += testOne(token + " N", degreeVal);
  rv += testOne(token + " S", -degreeVal);
  rv += testOne(token + " E",  degreeVal);
  rv += testOne(token + " W", -degreeVal);
  rv += testOne(token + " n",  degreeVal);
  rv += testOne(token + " s", -degreeVal);
  rv += testOne(token + " e",  degreeVal);
  rv += testOne(token + " w", -degreeVal);

  return rv;
}

int validValues()
{
  int rv = 0;

  rv += testCombinations("0", 0.0);

  rv += testCombinations("45", 45.0);
  rv += testCombinations("45.", 45.0);
  rv += testCombinations("45.0", 45.0);
  rv += testCombinations("45.00000000000000000000000000000000000000000", 45.0);

  double minuteAngle = 45.0 + 1.0 / 60.0;
  double secondAngle = 45.0 + 1.0 / 60.0 + 2.0 / 3600.0;
  rv += testCombinations("45 1", minuteAngle);
  rv += testCombinations("45 1 2", secondAngle);
  rv += testCombinations("45:1", minuteAngle);
  rv += testCombinations("45:1:2", secondAngle);
  rv += testCombinations("45,1", minuteAngle);
  rv += testCombinations("45,1,2", secondAngle);
  rv += testCombinations("45\t1", minuteAngle);
  rv += testCombinations("45\t1\t2", secondAngle);
  rv += testCombinations("45\n1", minuteAngle);
  rv += testCombinations("45\n1\n2", secondAngle);
  rv += testCombinations("45\u00B0", 45.0);
  rv += testCombinations("45.\u00B0", 45.0);
  rv += testCombinations("45.0\u00B0", 45.0);
  rv += testCombinations("45\u00B0 1'", minuteAngle);
  rv += testCombinations("45.\u00B0 1'", minuteAngle);
  rv += testCombinations("45.0\u00B0 1'", minuteAngle);
  rv += testCombinations("45\u00B0 1' 2\"", secondAngle);
  rv += testCombinations("45.\u00B0 1' 2\"", secondAngle);
  rv += testCombinations("45.0\u00B0 1' 2\"", secondAngle);
  rv += testCombinations("45\u00B0 01'", minuteAngle);
  rv += testCombinations("45.\u00B0 01'", minuteAngle);
  rv += testCombinations("45.0\u00B0 01'", minuteAngle);
  rv += testCombinations("45\u00B0 01' 02\"", secondAngle);
  rv += testCombinations("45.\u00B0 01' 02\"", secondAngle);
  rv += testCombinations("45.0\u00B0 01' 02\"", secondAngle);
  rv += testCombinations("45 1'", minuteAngle);
  rv += testCombinations("45. 1'", minuteAngle);
  rv += testCombinations("45.0 1'", minuteAngle);
  rv += testCombinations("45 1' 2\"", secondAngle);
  rv += testCombinations("45. 1' 2\"", secondAngle);
  rv += testCombinations("45.0 1' 2\"", secondAngle);
  rv += testCombinations("45 01'", minuteAngle);
  rv += testCombinations("45. 01'", minuteAngle);
  rv += testCombinations("45.0 01'", minuteAngle);
  rv += testCombinations("45 01' 02\"", secondAngle);
  rv += testCombinations("45. 01' 02\"", secondAngle);
  rv += testCombinations("45.0 01' 02\"", secondAngle);

  rv += testCombinations("90.0", 90.0);

  // The following pass but I don't think they should pass
  double angle;
  rv += SDK_ASSERT(simCore::getAngleFromDegreeString("45\u00B0 ' \"", false, angle) == 0);
  rv += SDK_ASSERT(simCore::areEqual(45.0, angle));
  rv += SDK_ASSERT(simCore::getAngleFromDegreeString("45\u00B0 1' \"", false, angle) == 0);
  rv += SDK_ASSERT(simCore::areEqual(minuteAngle, angle));

  return rv;
}

int invalidValues()
{
  int rv = 0;

  double angle;
  rv += SDK_ASSERT(simCore::getAngleFromDegreeString("", false, angle) == 1);
  rv += SDK_ASSERT(simCore::getAngleFromDegreeString(" ", false, angle) == 1);
  rv += SDK_ASSERT(simCore::getAngleFromDegreeString("Junk", false, angle) == 1);
  rv += SDK_ASSERT(simCore::getAngleFromDegreeString("\u00B0 ' \"", false, angle) == 1);

  return rv;
}

int testGetAngleFromDegreeString()
{
  int rv = 0;
  rv += SDK_ASSERT(validValues() == 0);
  rv += SDK_ASSERT(invalidValues() == 0);
  return rv;
}

int testGetDegreeAngleFromDegreeString()
{
  simCore::UniformVariable randomLat(-90 * simCore::DEG2RAD, 90 * simCore::DEG2RAD);
  simCore::UniformVariable randomLon(-180 * simCore::DEG2RAD, 180 * simCore::DEG2RAD);
  simCore::DiscreteUniformVariable random3(0, 2);
  simCore::DiscreteUniformVariable random2(0, 1);
  int rv = 0;
  int errCode = 0;
  double lat;
  double conv;
  std::string testString;
  // Test latitude first
  for (int k = 0; k < 1000; ++k)
  {
    lat = randomLat();
    int unitRandom = random3();
    simCore::GeodeticFormat angle = simCore::FMT_DEGREES_MINUTES_SECONDS;
    if (unitRandom == 0) angle = simCore::FMT_DEGREES_MINUTES;
    else if (unitRandom == 1) angle = simCore::FMT_DEGREES;
    int coinFlip = random2();
    testString = simCore::printLatitude(lat, angle, coinFlip != 0, 3, simCore::DEG_SYM_UNICODE);
    errCode = simCore::getAngleFromDegreeString(testString, false, conv);
    rv += SDK_ASSERT(simCore::areEqual(conv, lat*simCore::RAD2DEG, 0.001) && errCode == 0);
  }
  // Test longitude next
  for (int k = 0; k < 1000; ++k)
  {
    double lon = randomLon();
    int unitRandom = random3();
    simCore::GeodeticFormat angle = simCore::FMT_DEGREES_MINUTES_SECONDS;
    if (unitRandom == 0) angle = simCore::FMT_DEGREES_MINUTES;
    else if (unitRandom == 1) angle = simCore::FMT_DEGREES;
    int coinFlip = random2();
    testString = simCore::printLongitude(lon, angle, coinFlip != 0, 3, simCore::DEG_SYM_UNICODE);
    errCode = simCore::getAngleFromDegreeString(testString, false, conv);
    rv += SDK_ASSERT(simCore::areEqual(conv, lon*simCore::RAD2DEG, 0.001) && errCode == 0);
  }
  // Test exponential values
  testString = "-9.80676599278807E-03";
  errCode = simCore::getAngleFromDegreeString(testString, false, conv);
  rv += SDK_ASSERT(simCore::areEqual(conv, -9.80676599278807E-03, 0.0001) && errCode == 0);
  testString = "8.72305691976465E-02";
  errCode = simCore::getAngleFromDegreeString(testString, false, conv);
  rv += SDK_ASSERT(simCore::areEqual(conv, 8.72305691976465E-02, 0.0001) && errCode == 0);
  testString = "-4.10362106066276E-02";
  errCode = simCore::getAngleFromDegreeString(testString, false, conv);
  rv += SDK_ASSERT(simCore::areEqual(conv, -4.10362106066276E-02, 0.0001) && errCode == 0);
  testString = "3.43259430399202E+02";
  errCode = simCore::getAngleFromDegreeString(testString, false, conv);
  rv += SDK_ASSERT(simCore::areEqual(conv, 3.43259430399202E+02, 0.0001) && errCode == 0);
  testString = "-0.071708642471365E+02";
  errCode = simCore::getAngleFromDegreeString(testString, false, conv);
  rv += SDK_ASSERT(simCore::areEqual(conv, -0.071708642471365E+02, 0.0001) && errCode == 0);


  lat = 0.001;
  testString = simCore::printLatitude(lat, simCore::FMT_DEGREES, true, 7, simCore::DEG_SYM_UNICODE);
  errCode = simCore::getAngleFromDegreeString(testString, false, conv);
  rv += SDK_ASSERT(simCore::areEqual(conv, lat*simCore::RAD2DEG, 0.0001) && errCode == 0);

  testString = simCore::printLatitude(lat, simCore::FMT_RADIANS, true, 7, simCore::DEG_SYM_UNICODE);
  errCode = simCore::getAngleFromDegreeString(testString, false, conv);
  rv += SDK_ASSERT(simCore::areEqual(atof(testString.c_str()), lat, 0.0001) && errCode == 0);

  lat = -0.001;
  testString = simCore::printLatitude(lat, simCore::FMT_DEGREES, true, 7, simCore::DEG_SYM_UNICODE);
  errCode = simCore::getAngleFromDegreeString(testString, false, conv);
  rv += SDK_ASSERT(simCore::areEqual(conv, lat*simCore::RAD2DEG, 0.0001) && errCode == 0);

  testString = simCore::printLatitude(lat, simCore::FMT_RADIANS, true, 7, simCore::DEG_SYM_UNICODE);
  errCode = simCore::getAngleFromDegreeString(testString, false, conv);
  rv += SDK_ASSERT(simCore::areEqual(atof(testString.c_str()), lat, 0.0001) && errCode == 0);

  return rv;
}

int testAreAnglesEqual()
{
  int rv = 0;

  rv += SDK_ASSERT(simCore::areAnglesEqual(180.0*simCore::DEG2RAD, -180.0*simCore::DEG2RAD));
  rv += SDK_ASSERT(simCore::areAnglesEqual(361.0*simCore::DEG2RAD, 1.0*simCore::DEG2RAD));
  rv += SDK_ASSERT(simCore::areAnglesEqual(270.0*simCore::DEG2RAD, -90.0*simCore::DEG2RAD));
  rv += SDK_ASSERT(simCore::areAnglesEqual(725.0*simCore::DEG2RAD, 5.0*simCore::DEG2RAD));
  rv += SDK_ASSERT(simCore::areAnglesEqual(725.0*simCore::DEG2RAD, -355.0*simCore::DEG2RAD));

  rv += SDK_ASSERT(!simCore::areAnglesEqual(5.0*simCore::DEG2RAD, 5.1*simCore::DEG2RAD));
  rv += SDK_ASSERT(!simCore::areAnglesEqual(5.0*simCore::DEG2RAD, 5.1*simCore::DEG2RAD, 0.1*simCore::DEG2RAD));
  rv += SDK_ASSERT(simCore::areAnglesEqual(5.0*simCore::DEG2RAD, 5.0999*simCore::DEG2RAD, 0.1*simCore::DEG2RAD));

  simCore::Vec3 v1(0.0*simCore::DEG2RAD, 90.0*simCore::DEG2RAD, 180.0*simCore::DEG2RAD);
  simCore::Vec3 v2(-360.0*simCore::DEG2RAD, -270.0*simCore::DEG2RAD, -180.0*simCore::DEG2RAD);

  rv += SDK_ASSERT(simCore::v3AreAnglesEqual(v1, v2));

  return rv;
}

int testSim4481()
{
  int rv = 0;
  // Extra 3 are for the decimal and 2 places in the whole minutes
  std::string s = simCore::printLatitude(32.713727 * simCore::DEG2RAD, simCore::FMT_DEGREES_MINUTES, true, 5, simCore::DEG_SYM_UNICODE);
  rv += SDK_ASSERT(s == "32 42.82362");
  s = simCore::printLongitude(-119.2431765 * simCore::DEG2RAD, simCore::FMT_DEGREES_MINUTES, true, 5, simCore::DEG_SYM_UNICODE);
  rv += SDK_ASSERT(s == "-119 14.59059");

  // Try something with a 0 in tens place of minute, and 0 in decimals after
  s = simCore::printLatitude(32.0166666666 * simCore::DEG2RAD, simCore::FMT_DEGREES_MINUTES, true, 5, simCore::DEG_SYM_UNICODE);
  rv += SDK_ASSERT(s == "32 01.00000");
  s = simCore::printLongitude(-119.0166666666 * simCore::DEG2RAD, simCore::FMT_DEGREES_MINUTES, true, 5, simCore::DEG_SYM_UNICODE);
  rv += SDK_ASSERT(s == "-119 01.00000");
  // Try something with more decimals after the minute
  s = simCore::printLatitude(32.13888888 * simCore::DEG2RAD, simCore::FMT_DEGREES_MINUTES, true, 5, simCore::DEG_SYM_UNICODE);
  rv += SDK_ASSERT(s == "32 08.33333");
  s = simCore::printLongitude(-119.13888888 * simCore::DEG2RAD, simCore::FMT_DEGREES_MINUTES, true, 5, simCore::DEG_SYM_UNICODE);
  rv += SDK_ASSERT(s == "-119 08.33333");
  // Try with flag 0's
  s = simCore::printLatitude(32 * simCore::DEG2RAD, simCore::FMT_DEGREES_MINUTES, true, 5, simCore::DEG_SYM_UNICODE);
  rv += SDK_ASSERT(s == "32 00.00000");
  s = simCore::printLongitude(-119 * simCore::DEG2RAD, simCore::FMT_DEGREES_MINUTES, true, 5, simCore::DEG_SYM_UNICODE);
  rv += SDK_ASSERT(s == "-119 00.00000");

  // Fall back and test Degrees format with same values
  rv += SDK_ASSERT(simCore::printLatitude(32.713727 * simCore::DEG2RAD, simCore::FMT_DEGREES, true, 7, simCore::DEG_SYM_UNICODE) == "32.7137270");
  rv += SDK_ASSERT(simCore::printLongitude(-119.2431765 * simCore::DEG2RAD, simCore::FMT_DEGREES, true, 7, simCore::DEG_SYM_UNICODE) == "-119.2431765");
  // Try something with a 0 in tens place of minute, and 0 in decimals after
  rv += SDK_ASSERT(simCore::printLatitude(32.0166666666 * simCore::DEG2RAD, simCore::FMT_DEGREES, true, 7, simCore::DEG_SYM_UNICODE) == "32.0166667");
  rv += SDK_ASSERT(simCore::printLongitude(-119.0166666666 * simCore::DEG2RAD, simCore::FMT_DEGREES, true, 7, simCore::DEG_SYM_UNICODE) == "-119.0166667");
  // Try something with more decimals after the minute
  rv += SDK_ASSERT(simCore::printLatitude(32.13888888 * simCore::DEG2RAD, simCore::FMT_DEGREES, true, 7, simCore::DEG_SYM_UNICODE) == "32.1388889");
  rv += SDK_ASSERT(simCore::printLongitude(-119.13888888 * simCore::DEG2RAD, simCore::FMT_DEGREES, true, 7, simCore::DEG_SYM_UNICODE) == "-119.1388889");
  // Try with flag 0's
  rv += SDK_ASSERT(simCore::printLatitude(32 * simCore::DEG2RAD, simCore::FMT_DEGREES, true, 7, simCore::DEG_SYM_UNICODE) == "32.0000000");
  rv += SDK_ASSERT(simCore::printLongitude(-119 * simCore::DEG2RAD, simCore::FMT_DEGREES, true, 7, simCore::DEG_SYM_UNICODE) == "-119.0000000");

  // Now try with DMS format
  rv += SDK_ASSERT(simCore::printLatitude(32.713727 * simCore::DEG2RAD, simCore::FMT_DEGREES_MINUTES_SECONDS, true, 3, simCore::DEG_SYM_UNICODE) == "32 42 49.417");
  rv += SDK_ASSERT(simCore::printLongitude(-119.2431765 * simCore::DEG2RAD, simCore::FMT_DEGREES_MINUTES_SECONDS, true, 3, simCore::DEG_SYM_UNICODE) == "-119 14 35.435");
  // Try something with a 0 in tens place of minute, and 0 in decimals after
  rv += SDK_ASSERT(simCore::printLatitude(32.0166666666 * simCore::DEG2RAD, simCore::FMT_DEGREES_MINUTES_SECONDS, true, 3, simCore::DEG_SYM_UNICODE) == "32 01 00.000");
  rv += SDK_ASSERT(simCore::printLongitude(-119.0166666666 * simCore::DEG2RAD, simCore::FMT_DEGREES_MINUTES_SECONDS, true, 3, simCore::DEG_SYM_UNICODE) == "-119 01 00.000");
  // Try something with more decimals after the minute
  rv += SDK_ASSERT(simCore::printLatitude(32.13888888 * simCore::DEG2RAD, simCore::FMT_DEGREES_MINUTES_SECONDS, true, 3, simCore::DEG_SYM_UNICODE) == "32 08 20.000");
  rv += SDK_ASSERT(simCore::printLongitude(-119.13888888 * simCore::DEG2RAD, simCore::FMT_DEGREES_MINUTES_SECONDS, true, 3, simCore::DEG_SYM_UNICODE) == "-119 08 20.000");
  // Try with flag 0's
  rv += SDK_ASSERT(simCore::printLatitude(32 * simCore::DEG2RAD, simCore::FMT_DEGREES_MINUTES_SECONDS, true, 3, simCore::DEG_SYM_UNICODE) == "32 00 00.000");
  rv += SDK_ASSERT(simCore::printLongitude(-119 * simCore::DEG2RAD, simCore::FMT_DEGREES_MINUTES_SECONDS, true, 3, simCore::DEG_SYM_UNICODE) == "-119 00 00.000");

  // Try a rounding test with minutes format
  rv += SDK_ASSERT(simCore::printLatitude(31.9999999 * simCore::DEG2RAD, simCore::FMT_DEGREES_MINUTES, true, 0, simCore::DEG_SYM_UNICODE) == "32 00");
  rv += SDK_ASSERT(simCore::printLatitude(31.9999999 * simCore::DEG2RAD, simCore::FMT_DEGREES_MINUTES, true, 1, simCore::DEG_SYM_UNICODE) == "32 00.0");
  rv += SDK_ASSERT(simCore::printLatitude(31.9999999 * simCore::DEG2RAD, simCore::FMT_DEGREES_MINUTES, true, 2, simCore::DEG_SYM_UNICODE) == "32 00.00");
  rv += SDK_ASSERT(simCore::printLatitude(31.9999999 * simCore::DEG2RAD, simCore::FMT_DEGREES_MINUTES, true, 3, simCore::DEG_SYM_UNICODE) == "32 00.000");

  return rv;
}

// Convert DMS into a radian
double dmsAsRadian(double deg, double min, double sec)
{
  double val = std::abs(deg) + min / 60.0 + sec / 3600.0;
  if (deg < 0) val = -val;
  return simCore::DEG2RAD * val;
}

int testAngle(int deg, int min, int sec, double offset, const std::string& degStr, const std::string& degMinStr, const std::string& degMinSecStr)
{
  int rv = 0;
  double val = dmsAsRadian(deg, min, sec) + simCore::DEG2RAD * offset;
  std::string d = simCore::printLatitude(val, simCore::FMT_DEGREES, true, 0, simCore::DEG_SYM_NONE);
  if (d != degStr)
  {
    rv += 1;
    std::cerr << "ERROR: " << val * simCore::RAD2DEG << " in DEGREES as " << d << "; expected " << degStr << std::endl;
  }
  std::string dm = simCore::printLatitude(val, simCore::FMT_DEGREES_MINUTES, true, 0, simCore::DEG_SYM_NONE);
  if (dm != degMinStr)
  {
    rv += 1;
    std::cerr << "ERROR: " << val * simCore::RAD2DEG << " in DEGREES_MINUTES as " << dm << "; expected " << degMinStr << std::endl;
  }
  std::string dms = simCore::printLatitude(val, simCore::FMT_DEGREES_MINUTES_SECONDS, true, 0, simCore::DEG_SYM_NONE);
  if (dms != degMinSecStr)
  {
    rv += 1;
    std::cerr << "ERROR: " << val * simCore::RAD2DEG << " in DEGREES_MINUTES_SECONDS as " << dms << "; expected " << degMinSecStr << std::endl;
  }

  return rv;
}

// Super Form Platform Data frame reports "33 13 00" as "33 12 60" when using DMS
int testSim1755()
{
  int rv = 0;
  rv += SDK_ASSERT(0 == testAngle(33, 13, 59, 0.988 / 3600, "33", "33 14", "33 14 00"));

  rv += SDK_ASSERT(0 == testAngle(32, 0, 0, 0.0, "32", "32 00", "32 00 00"));
  rv += SDK_ASSERT(0 == testAngle(32, 1, 0, 0.0, "32", "32 01", "32 01 00"));
  rv += SDK_ASSERT(0 == testAngle(32, 1, 1, 0.0, "32", "32 01", "32 01 01"));
  rv += SDK_ASSERT(0 == testAngle(32, 30, 0, 0.0, "33", "32 30", "32 30 00"));
  rv += SDK_ASSERT(0 == testAngle(32, 1, 30, 0.0, "32", "32 02", "32 01 30"));
  rv += SDK_ASSERT(0 == testAngle(32, 30, 30, 0.0, "33", "32 31", "32 30 30"));
  rv += SDK_ASSERT(0 == testAngle(32, 59, 30, 0.0, "33", "33 00", "32 59 30"));
  rv += SDK_ASSERT(0 == testAngle(32, 59, 59, 0.0, "33", "33 00", "32 59 59"));
  // Small epsilon, testing round-up
  rv += SDK_ASSERT(0 == testAngle(33, 0, 0, -0.00000001, "33", "33 00", "33 00 00"));

  rv += SDK_ASSERT(0 == testAngle(-32, 0, 0, 0.0, "-32", "-32 00", "-32 00 00"));
  rv += SDK_ASSERT(0 == testAngle(-32, 1, 0, 0.0, "-32", "-32 01", "-32 01 00"));
  rv += SDK_ASSERT(0 == testAngle(-32, 1, 1, 0.0, "-32", "-32 01", "-32 01 01"));
  rv += SDK_ASSERT(0 == testAngle(-32, 30, 0, 0.0, "-33", "-32 30", "-32 30 00"));
  rv += SDK_ASSERT(0 == testAngle(-32, 1, 30, 0.0, "-32", "-32 02", "-32 01 30"));
  rv += SDK_ASSERT(0 == testAngle(-32, 30, 30, 0.0, "-33", "-32 31", "-32 30 30"));
  rv += SDK_ASSERT(0 == testAngle(-32, 59, 30, 0.0, "-33", "-33 00", "-32 59 30"));
  rv += SDK_ASSERT(0 == testAngle(-32, 59, 59, 0.0, "-33", "-33 00", "-32 59 59"));
  // Small epsilon, testing round-down
  rv += SDK_ASSERT(0 == testAngle(-33, 0, 0, 0.00000001, "-33", "-33 00", "-33 00 00"));
  return rv;
}

// Test ASI parsing of latitude and longitude
int testSim2511()
{
  int rv = 0;
  double degAng;
  rv = SDK_ASSERT(simCore::getAngleFromDegreeString("!", false, degAng) != 0);
  if (rv)
  {
    std::cout << "testSim2511 failed with bad input: !" << std::endl;
    return 1;
  }
  rv = SDK_ASSERT(simCore::getAngleFromDegreeString("fail", false, degAng) != 0);
  if (rv)
  {
    std::cout << "testSim2511 failed with bad input: fail" << std::endl;
    return 1;
  }
  rv = SDK_ASSERT(simCore::getAngleFromDegreeString("a", false, degAng) != 0);
  if (rv)
  {
    std::cout << "testSim2511 failed with bad input: a" << std::endl;
    return 1;
  }
  rv = SDK_ASSERT(simCore::getAngleFromDegreeString("-INF", false, degAng) != 0);
  if (rv)
  {
    std::cout << "testSim2511 failed with bad input: -INF" << std::endl;
    return 1;
  }
  rv = SDK_ASSERT(simCore::getAngleFromDegreeString("INF", false, degAng) != 0);
  if (rv)
  {
    std::cout << "testSim2511 failed with bad input: INF" << std::endl;
    return 1;
  }
  rv = SDK_ASSERT(simCore::getAngleFromDegreeString("-1.#INF", false, degAng) != 0);
  if (rv)
  {
    std::cout << "testSim2511 failed with bad input: -1.#INF" << std::endl;
    return 1;
  }
  rv = SDK_ASSERT(simCore::getAngleFromDegreeString("1.#INF", false, degAng) != 0);
  if (rv)
  {
    std::cout << "testSim2511 failed with bad input: 1.#INF" << std::endl;
    return 1;
  }
  rv = SDK_ASSERT(simCore::getAngleFromDegreeString("", false, degAng) != 0);
  if (rv)
  {
    std::cout << "testSim2511 failed with bad input: empty string" << std::endl;
    return 1;
  }
  std::string testString = "abc ";
  testString += simCore::printLatitude(22, simCore::FMT_DEGREES, true, 7, simCore::DEG_SYM_UNICODE);
  rv = SDK_ASSERT(simCore::getAngleFromDegreeString(testString, false, degAng) != 0);
  if (rv)
  {
    std::cout << "testSim2511 failed with bad input: " << testString << std::endl;
    return 1;
  }
  testString = "abc ";
  testString += simCore::printLatitude(22, simCore::FMT_DEGREES, false, 7, simCore::DEG_SYM_UNICODE);
  rv = SDK_ASSERT(simCore::getAngleFromDegreeString(testString, false, degAng) != 0);
  if (rv)
  {
    std::cout << "testSim2511 failed with bad input: " << testString << std::endl;
    return 1;
  }

  return rv;
}

}

int AngleTest(int argc, char* argv[])
{
  simCore::checkVersionThrow();
  int rv = 0;

  rv += testGetAngleFromDegreeString();
  rv += testGetDegreeAngleFromDegreeString();
  rv += testAreAnglesEqual();
  rv += testSim1755();
  rv += testSim2511();
  rv += testSim4481();

  return rv;
}
