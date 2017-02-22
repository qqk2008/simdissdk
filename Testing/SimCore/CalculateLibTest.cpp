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
#include "simCore/Calc/CalcRelAbs.h"
#include "simCore/Calc/CoordinateConverter.h"
#include "simCore/Calc/Math.h"

#include <vector>
#include <string>
#include <fstream>
#include <cstdlib>
#include <iostream>
#include <iomanip>

using namespace std;
using namespace simCore;

//----------------------------------------------------------------------------
static bool almostEqual(double value1, double value2, double epsilon=1e-4)
{
  if (!areEqual(value1, value2, epsilon))
  {
    cerr << "FAILURE" <<endl;
    cerr << setprecision(16) << "    " << value1 << " != " << value2 << " delta: " << value1 - value2 << endl;
    return false;
  }
  return true;
}

//----------------------------------------------------------------------------
//void testCalcCPRFromNEDVelVector(double *nedVel, double *result)
//{
//  cerr << "Testing calcCPRFromENUVelVector ++++++++++++++++++++++++++\n";
//
//  double cpr[3];
//
//  calcCPRFromENUVelVector(nedVel, cpr);
//  almostEqual(cpr[0], result[0]);
//  almostEqual(cpr[1], result[1]);
//  almostEqual(cpr[2], result[2]);
//}

//void testCalculateRelAng(double *fromcpr, double *dirvect, double *result)
//{
//  cerr << "Testing calculateRelAng ++++++++++++++++++++++++++\n";
//
//  double azim;
//  double elev;
//  double compositeAngle;
//
//  calculateRelAng(dirvect, fromcpr, &azim, &elev, &compositeAngle);
//  almostEqual(azim, result[0]);
//  almostEqual(elev, result[1]);
//  almostEqual(compositeAngle, result[2]);
//}

static int testCalculateRelAzEl(double *from, double *to, EEarthModelCalculations earth, CoordinateConverter coordConvert, double *result)
{
  cerr << "calculateRelAzEl +++++++++++++ ";

  if (earth == ePerfectSphere)
  {
    cerr<<"calculation not valid for Earth Model"<<endl;
    return 1;
  }

  double azim;
  double elev;
  double compositeAngle;

  calculateRelAzEl(from, to, &azim, &elev, &compositeAngle, earth, &coordConvert);

  if (almostEqual(azim, result[0]) &&
      almostEqual(elev, result[1]) &&
      almostEqual(compositeAngle, result[2]))
  {
    cerr << "successful"<<endl;
    return 0;
  }
  return 1;
}

static int testCalculateAbsAzEl(double *from, double *to, EEarthModelCalculations earth, CoordinateConverter coordConvert, double *result)
{
  cerr << "calculateAbsAzEl +++++++++++++ ";

  double azim;
  double elev;
  double compositeAngle;

  calculateAbsAzEl(Vec3(from[0], from[1], from[2]), to, &azim, &elev, &compositeAngle, earth, &coordConvert);

  if (almostEqual(azim, result[0]) &&
      almostEqual(elev, result[1]) &&
      almostEqual(compositeAngle, result[2]))
  {
    cerr << "successful"<<endl;
    return 0;
  }
  return 1;
}

static int testCalculateSlant(double *from, double *to, EEarthModelCalculations earth, CoordinateConverter coordConvert, double *result)
{
  cerr << "calculateSlant +++++++++++++++ ";

  const double slant = calculateSlant(Vec3(from[0], from[1], from[2]), to, earth, &coordConvert);
  if (almostEqual(slant, result[0]))
  {
    cerr << "successful"<<endl;
    return 0;
  }
  return 1;
}

static int testCalculateGroundDist(double *from, double *to, EEarthModelCalculations earth, CoordinateConverter coordConvert, double *result)
{
  cerr << "calculateGroundDist ++++++++++ ";

  if (earth == ePerfectSphere)
  {
    cerr<<"calculation not valid for Earth Model"<<endl;
    return 1;
  }

  const double groundDist = calculateGroundDist(from, to, earth, &coordConvert);
  if (almostEqual(groundDist, result[0]))
  {
    cerr << "successful"<<endl;
    return 0;
  }
  return 1;
}

static int testCalculateAltitude(double *from, double *to, EEarthModelCalculations earth, CoordinateConverter coordConvert, double *result)
{
  cerr << "calculateAltitude ++++++++++++ ";

  if (earth == ePerfectSphere)
  {
    cerr << "calculation not valid for Earth Model" << endl;
    return 1;
  }

  const double altitude = calculateAltitude(Vec3(from[0], from[1], from[2]),
                                            Vec3(to[0], to[1], to[2]), earth, &coordConvert);
  if (almostEqual(altitude, result[0]))
  {
    cerr << "successful"<<endl;
    return 0;
  }
  return 1;
}

static int testCalculateDRCRDownValue(double *from, double *to, EEarthModelCalculations earth, CoordinateConverter coordConvert, double *result)
{
  cerr << "calculateDRCRDownValue +++++++ ";

  double downRng;
  double crossRng;
  double downValue;

  calculateDRCRDownValue(from, to, earth, &coordConvert, &downRng, &crossRng, &downValue);

  if (almostEqual(downRng, result[0]) &&
     almostEqual(crossRng, result[1]) &&
     almostEqual(downValue, result[2]))
  {
    cerr << "successful" << endl;
    return 0;
  }
  return 1;
}

static int testCalculateGeodesicDRCR(double *from, double *to, EEarthModelCalculations earth, CoordinateConverter coordConvert, double *result)
{
  cerr << "calculateGeodesicDRCR ++++++++ ";

  double downRng;
  double crossRng;

  calculateGeodesicDRCR(from, to, &downRng, &crossRng);

  if (almostEqual(downRng, result[0]) &&
      almostEqual(crossRng, result[1]))
  {
    cerr << "successful" << endl;
    return 0;
  }
  return 1;
}

static int testCalculateTotalVelocity(double *from, double *to, double deltaTime, EEarthModelCalculations earth, double *result)
{
  return 0; //Ned, broken
/*
  cerr << "calculateTotalVelocity +++++++ ";

  double velVec[3];

  CalcVelocityFromLLA(from, to, deltaTime, &velVec[0]);
  if(almostEqual(velVec[0], result[0]) &&
     almostEqual(velVec[1], result[1]) &&
     almostEqual(velVec[2], result[2]))
  {
    cerr << "successful"<<endl;
    return 0;
  }
  return 1;
*/
}

static int testCalculateClosingVelocity(double *from, double *to, EEarthModelCalculations earth, CoordinateConverter coordConvert, double *result)
{
  cerr << "calculateClosingVelocity +++++ ";

  if (earth == ePerfectSphere)
  {
    cerr << "calculation not valid for Earth Model" << endl;
    return 1;
  }

  const double velocity = calculateClosingVelocity(from, to, earth, &coordConvert, &from[6], &to[6]);

  if (almostEqual(velocity, result[0]))
  {
    cerr << "successful"<<endl;
    return 0;
  }
  return 1;
}

static int testCalculateVelocityDelta(double *from, double *to, EEarthModelCalculations earth, CoordinateConverter coordConvert, double *result)
{
  cerr << "calculateVelocityDelta +++++++ ";

  if (earth == ePerfectSphere)
  {
    cerr<<"calculation not valid for Earth Model"<<endl;
    return 1;
  }

  const double velocity = calculateVelocityDelta(from, to, earth, &coordConvert, &from[6], &to[6]);
  if (almostEqual(velocity, result[0]))
  {
    cerr << "successful"<<endl;
    return 0;
  }
  return 1;
}

static void printInstructions()
{
  static bool seenInstructions = false;
  if (!seenInstructions)
  {
    seenInstructions = true;
    cout << "Input File Format:"<<endl<<endl;
    cout << "[Calculation][CoordinateSystem] [ReferenceOrigin]"<<endl;
    cout << "[Arg1] [Arg2] ... [ArgN]"<<endl;
    cout << "[ExpectedResult1] [ExpectedResult2] ... [ExpectedResultN]"<<endl<<endl;
  }
}

/////////////////////////////////////////////////////////////////////////////
//Takes an input file with the following format:
//
//[Calculation][CoordinateSystem] [ReferenceOrigin]
//[Arg1] [Arg2] ... [ArgN]
//[ExpectedResult1] [ExpectedResult2] ... [ExpectedResultN]
//
//Where CoordinateSystem = WGS84|FlatEarth|PerfectSphere|TangentPlaneWGS84
//
//Following calculations are available:
//
//_______________________________________________________________________
//|Calculation          | Input Arguments      | # Results             |
//-----------------------------------------------------------------------
//| Slant               | from[3] to[3]        | 1                     |
//| AbsAzEl             | from[3] to[3]        | 3 [Az, El, Composite] |
//| RelAzEl             | from[6] to[3]        | 3 [Az, El, Composite] |
//| Altitude            | from[3] to[3]        | 1                     |
//| GroundDist          | from[3] to[3]        | 1                     |
//| GeodesicDRCR        | from[6] to[3]        | 2 [DR, CR]            |
//| VelocityDelta       | from[9] to[9]        | 1                     |
//| TotalVelocity       | from[9] to[9]        | 3 [Velocity Vector]   |
//| DRCRDownValue       | from[6] to[3]        | 3 [DR, CR, DownValue] |
//| ClosingVelocity     | from[9] to[9]        | 1                     |
//| LinearInterpolation | from[10] to[10] time | 3 [Velocity Vector]   |
//
//
/////////////////////////////////////////////////////////////////////////////

//===========================================================================
int CalculateLibTest(int argc, char* argv[])
{
  if (argc < 2)
  {
    cout << "Usage: " << argv[0] << " [filename]" << endl;
    return -1;
  }

  // Read the test data
  ifstream fd(argv[1]);
  if (!fd)
  {
    cout << "Error opening file " << argv[1] << endl;
    return -1;
  }

  int rv = 0;

  double refOrigin[3];
  double from[10];
  double to[10];
  double time;
  double result[3];
  EEarthModelCalculations earth = ePerfectSphere;

  string test;
  while (fd >> test)
  {
    // set coordinate system / reference frame
    if (test.compare(simCore::sdkMax((int)test.length()-17, 0), test.length()-1, "TangentPlaneWGS84") == 0)
    {
      if (earth != eTangentPlaneWGS84)
      {
         cerr << "Earth Model: TangentPlaneWGS84"<<endl;
         earth = eTangentPlaneWGS84;
      }
    }
    else if (test.compare(simCore::sdkMax((int)test.length()-5, 0), test.length()-1, "WGS84") == 0)
    {
      if (earth != eWGS84)
      {
        cerr << "Earth Model: WGS84"<<endl;
        earth = eWGS84;
      }
    }
    else if (test.compare(simCore::sdkMax((int)test.length()-9, 0), test.length()-1, "FlatEarth") == 0)
    {
      if (earth != eFlatEarth)
      {
        cerr << "Earth Model: FlatEarth"<<endl;
        earth = eFlatEarth;
      }
    }
    else if (test.compare(simCore::sdkMax((int)test.length()-13, 0), test.length()-1, "PerfectSphere") == 0)
    {
      if (earth != ePerfectSphere)
      {
        cerr << "Earth Model: PerfectSphere"<<endl;
        earth = ePerfectSphere;
      }
    }
    else
    {
      cout << "Incorrect input file format:"<<" missing valid coordinate system / reference frame"<<endl;
      printInstructions();
      rv++;
    }

    cerr<<"  ";

    // get and set Reference Origin
    fd >> refOrigin[0] >> refOrigin[1] >> refOrigin[2];
    CoordinateConverter coordConvert;
    coordConvert.setReferenceOrigin(refOrigin[0], refOrigin[1], refOrigin[2]);

    // set to/from and run relevant test
    if (test.compare(0, 5, "Slant")==0)
    {
      fd >> from[0] >> from[1] >> from[2] >> to[0] >> to[1] >> to[2];
      fd >> result[0];
      rv += testCalculateSlant(from, to, earth, coordConvert, result);
    }
    else if (test.compare(0, 7, "AbsAzEl")==0)
    {
      fd >> from[0] >> from[1] >> from[2] >> to[0] >> to[1] >> to[2];
      fd >> result[0] >> result[1] >> result[2];
      rv += testCalculateAbsAzEl(from, to, earth, coordConvert, result);
    }
    else if (test.compare(0, 7, "RelAzEl")==0)
    {
      fd >> from[0] >> from[1] >> from[2] >> from[3] >> from[4] >> from[5];
      fd >> to[0] >> to[1] >> to[2];
      fd >> result[0] >> result[1] >> result[2];
      rv += testCalculateRelAzEl(from, to, earth, coordConvert, result);
    }
    else if (test.compare(0, 8, "Altitude")==0)
    {
      fd >> from[0] >> from[1] >> from[2] >> to[0] >> to[1] >> to[2];
      fd >> result[0];
      rv += testCalculateAltitude(from, to, earth, coordConvert, result);
    }
    else if (test.compare(0, 10, "GroundDist")==0)
    {
      fd >> from[0] >> from[1] >> from[2] >> to[0] >> to[1] >> to[2];
      fd >> result[0];
      rv += testCalculateGroundDist(from, to, earth, coordConvert, result);
    }
    else if (test.compare(0, 12, "GeodesicDRCR")==0)
    {
      fd >> from[0] >> from[1] >> from[2] >> from[3] >> from[4] >> from[5];
      fd >> to[0] >> to[1] >> to[2];
      fd >> result[0] >> result[1];
      rv += testCalculateGeodesicDRCR(from, to, earth, coordConvert, result);
    }
    else if (test.compare(0, 13, "VelocityDelta")==0)
    {
      fd >> from[0] >> from[1] >> from[2] >> from[3] >> from[4] >> from[5] >> from[6] >> from[7] >> from[8];
      fd >> to[0] >> to[1] >> to[2] >> to[3] >> to[4] >> to[5] >> to[6] >> to[7] >> to[8];
      fd >> result[0];
      rv += testCalculateVelocityDelta(from, to, earth, coordConvert, result);
    }
    else if (test.compare(0, 13, "TotalVelocity")==0)
    {
      fd >> from[0] >> from[1] >> from[2] >> from[3] >> from[4] >> from[5] >> from[6] >> from[7] >> from[8];
      fd >> to[0] >> to[1] >> to[2] >> to[3] >> to[4] >> to[5] >> to[6] >> to[7] >> to[8];
      fd >> time;
      fd >> result[0] >> result[1] >> result[2];
      rv += testCalculateTotalVelocity(from, to, time, earth, result);
    }
    else if (test.compare(0, 13, "DRCRDownValue")==0)
    {
      fd >> from[0] >> from[1] >> from[2] >> from[3] >> from[4] >> from[5];
      fd >> to[0] >> to[1] >> to[2];
      fd >> result[0] >> result[1] >> result[2];
      rv += testCalculateDRCRDownValue(from, to, earth, coordConvert, result);
    }
    else if (test.compare(0, 15, "ClosingVelocity")==0)
    {
      fd >> from[0] >> from[1] >> from[2] >> from[3] >> from[4] >> from[5] >> from[6] >> from[7] >> from[8];
      fd >> to[0] >> to[1] >> to[2] >> to[3] >> to[4] >> to[5] >> to[6] >> to[7] >> to[8];
      fd >> result[0];
      rv += testCalculateClosingVelocity(from, to, earth, coordConvert, result);
    }
    else
    {
      cout << "Command not valid: " << test;
      printInstructions();
      rv++;
    }
  } //while( fd >> test )

  return rv;
}

int main(int argc, char **argv)
{
  int rv = CalculateLibTest(argc, argv);
  if (rv)
    cerr << "Test failed" << std::endl;

  return rv;
}

