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

#ifndef SIMCORE_CALC_MGRS_H
#define SIMCORE_CALC_MGRS_H

#include <string>
#include "simCore/Common/Export.h"

namespace simCore {

/**
 * @brief Methods for conversion from MGRS/UTM/UPS to geodetic coordinates.
 *
 * Note: Several functions have been repurposed from software provided by the
 *       White Sands Missile Range (WSMR) and from the GEOTRANS library.
 *       Source code for GEOTRANS can be found here: http://earth-info.nga.mil/GandG/geotrans/
 *       GEOTRANS license can be found here: http://earth-info.nga.mil/GandG/geotrans/docs/MSP_GeoTrans_Terms_of_Use.pdf
 */
class SDKCORE_EXPORT Mgrs
{
public:

  /// Enumeration for parameter passed to and from the UTM/UPS conversion methods.
  enum Hemisphere
  {
    UPS_SOUTH,
    UPS_NORTH
  };

  /**
  * Converts an MGRS coordinate to geodetic coordinates.
  * Note that the function is currently defined only for values that convert to latitudes of less than 80 degrees
  * south or 84 degrees north, as it uses converts first to UTM (see convertUtmToGeodetic).
  * @param[in ] mgrs MGRS coordinate string
  * @param[out] lat Latitude of resulting conversion in radians
  * @param[out] lon Longitude of resulting conversion in radians
  * @param[out] err Optional pointer to error string
  * @return 0 if conversion is successful, non-zero otherwise
  */
  static int convertMgrsToGeodetic(const std::string& mgrs, double& lat, double& lon, std::string* err = NULL);

  /**
  * Breaks an MGRS coordinate string into its components.
  *
  * @param[in ] mgrs MGRS coordinate string
  * @param[out] zone UTM zone, should be in the range of 1-60. If 0, then the coordinate is in UPS format.
  * @param[out] gzdLetters GZD of the UPS coordinate, minus the zone (will always be 3 characters)
  * @param[out] easting Easting portion of position within grid, output resolution of 1 meter or finer
  * @param[out] northing Northing portion of position within grid, output resolution of 1 meter or finer
  * @param[out] err Optional pointer to error string
  * @return 0 if conversion is successful, non-zero otherwise
  */
  static int breakMgrsString(const std::string& mgrs, int& zone, std::string& gzdLetters, double& easting,
    double& northing, std::string* err = NULL);

  /**
  * Converts an MGRS coordinate to geodetic coordinates.
  * This is currently used for grid coordinates that would convert to latitudes of less than 80 degrees
  * south or 84 degrees north.
  *
  * @param[in ] zone UTM zone, should be in the range of 1-60
  * @param[in ] gzdLetters GZD of the UPS coordinate, minus the zone (should always be 3 characters)
  * @param[in ] mgrsEasting Easting portion of MGRS coordinate position within grid
  * @param[in ] mgrsNorthing Northing portion of MGRS coordinate position within grid
  * @param[out] hemisphere Hemisphere (north/south) of the coordinate
  * @param[out] utmEasting Easting portion of resulting UTM coordinate
  * @param[out] utmNorthing Northing portion of resulting UTM coordinate
  * @param[out] err Optional pointer to error string
  * @return 0 if conversion is successful, non-zero otherwise
  */
  static int convertMgrstoUtm(int zone, const std::string& gzdLetters, double mgrsEasting, double mgrsNorthing,
    Hemisphere& hemisphere, double& utmEasting, double& utmNorthing, std::string* err = NULL);

  /**
  * Converts a UTM coordinate to geodetic coordinates.
  * Note that the function is defined only for values that convert to latitudes of less than 80 degrees
  * south or 84 degrees north, as per the UTM standard. Basic range checking is done, but the user is
  * expected to provide valid easting and northing values.
  *
  * @param[in ] zone UTM zone, should be in the range of 1-60
  * @param[in ] hemisphere Hemisphere (north/south) of the coordinate
  * @param[in ] easting Easting portion of position within grid
  * @param[in ] northing Northing portion of position within grid
  * @param[out] lat Latitude of resulting conversion in radians
  * @param[out] lon Longitude of resulting conversion in radians
  * @param[out] err Optional pointer to error string
  * @return 0 if conversion is successful, non-zero otherwise
  */
  static int convertUtmToGeodetic(int zone, Hemisphere hemisphere, double easting, double northing, double& lat, double& lon, std::string* err = NULL);

  /**
  * Converts an MGRS coordinate to UPS coordinates.
  * This is used for grid coordinates that would convert to latitudes greater than 80 degrees south or 84
  * degrees north. UTM zone should always be 0 and is thus not passed in as a parameter to the function.
  *
  * @param[in ] gzdLetters GZD of the MGRS coordinate
  * @param[in ] mgrsEasting Easting portion of position within MGRS grid
  * @param[in ] mgrsNorthing Northing portion of position within MGRS grid
  * @param[out] hemisphere Hemisphere for UPS coordinate: UPS_SOUTH or UPS_NORTH
  * @param[out] upsEasting Easting portion of position within UPS grid
  * @param[out] upsNorthing Northing portion of position within UPS grid
  * @param[out] err Optional pointer to error string
  * @return 0 if conversion is successful, non-zero otherwise
  */
  static int convertMgrsToUps(const std::string& gzdLetters, double mgrsEasting, double mgrsNorthing,
    Hemisphere& hemisphere, double& upsEasting, double& upsNorthing, std::string* err = NULL);

  /**
  * Converts a UPS coordinate to geodetic coordinates.
  * Note that this method should only be used for grid coordinates that would convert to latitudes
  * greater than 80 degrees south or 84 degrees north, as per the UTM standard. Coordinates closer to
  * the equator may have relatively significant errors in latitude.
  *
  * @param[in ] hemisphere Hemisphere for UPS coordinate: UPS_SOUTH or UPS_NORTH
  * @param[in ] easting Easting portion of position within grid
  * @param[in ] northing Northing portion of position within grid
  * @param[out] lat Latitude of resulting conversion in radians
  * @param[out] lon Longitude of resulting conversion in radians
  * @param[out] err Optional pointer to error string
  * @return 0 if conversion is successful, non-zero otherwise
  */
  static int convertUpsToGeodetic(Hemisphere hemisphere, double easting, double northing, double& lat, double& lon, std::string* err = NULL);

private:

  struct Latitude_Band
  {
    /// minimum northing for latitude band
    double minNorthing;
    /// latitude band northing offset
    double northingOffset;
  };

  struct UPS_Constants
  {
    /// Grid column letter range - low value
    char gridColumnLowValue;
    /// Grid column letter range - high value
    char gridColumnHighValue;
    /// Grid row letter range - high value
    char gridRowHighValue;
    /// False easting based on grid column letter
    double falseEasting;
    /// False northing based on grid row letter
    double falseNorthing;
  };

  /*
  * Receives a latitude band letter and returns the minimum northing and northing offset for that
  * latitude band letter.
  *
  * @param[in ] bandLetter Latitude band letter
  * @param[out] minNorthing Minimum northing for the given band letter
  * @param[out] northingOffset Latitude band northing offset
  * @return 0 on valid band letter input, non-zero otherwise
  */
  static int getLatitudeBandMinNorthing_(char bandLetter, double& minNorthing, double& northingOffset);

  /*
  * Sets the letter range used for the grid zone column letter in the MGRS coordinate string, based on the
  * UTM zone number. It also sets the pattern offset using the UTM zone's pattern.
  *
  * @param[in ] zone UTM Zone number
  * @param[out] columnLetterLowValue Lower bound for the grid column letter
  * @param[out] columnLetterHighValue Upper bound for the grid column letter
  * @param[out] patternOffset Offset to the grid northing value based on the pattern of the UTM zone number
  */
  static void getGridValues_(int zone, char& columnLetterLowValue, char& columnLetterHighValue, double& patternOffset);

  /// Computes the hyperbolic arctangent of the given input.
  static double atanh_(double x);
};

} // Namespace simCore

#endif
