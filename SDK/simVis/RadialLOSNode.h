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
#ifndef SIMVIS_RADIAL_LOS_NODE_H
#define SIMVIS_RADIAL_LOS_NODE_H

#include "simVis/RadialLOS.h"

#include "osgEarth/MapNode"
#include "osgEarth/GeoData"
#include "osgEarthAnnotation/GeoPositionNode"

#include "osg/Geode"
#include "osg/Geometry"


namespace simVis
{
  /**
   * Radial line-of-sight node renders an LOS display corresponding to
   * the data in a RadialLOS structure.
   */
  class SDKVIS_EXPORT RadialLOSNode : public osgEarth::Annotation::GeoPositionNode
  {
  public:
    /**
     * Constructs a new LOS node.
     * @param[in ] mapNode MapNode to which to attach this object
     */
    RadialLOSNode(osgEarth::MapNode* mapNode);

  public:

    /**
     * Sets the center position of this object.
     * @param[in ] coord Origin of the LOS display
     */
    virtual bool setCoordinate(const simCore::Coordinate& coord);

    /**
     * Gets the center/origin coordinate.
     */
    const simCore::Coordinate& getCoordinate() const { return coord_; }

    /**
     * Sets the data model to visualize.
     * @param[in ] los LOS data model
     */
    void setDataModel(const RadialLOS& los);

    /**
     * Gets the data model this node is visualizing
     * @return Radial LOS data model
     */
    const RadialLOS& getDataModel() const { return los_; }

    /**
     * Sets the "visible" color
     * param[in ] color for visible areas (rgba, [0..1])
     */
    void setVisibleColor(const osg::Vec4& color);

    /**
     * Gets the "visible" color
     */
    const osg::Vec4& getVisibleColor() const { return visibleColor_; }

    /**
     * Sets the "obstructed" color
     * @param[in ] color for obstructed areas (rgba, [0..1])
     */
    void setObstructedColor(const osg::Vec4& color);

    /**
     * Gets the "obstructed" color
     */
    const osg::Vec4& getObstructedColor() const { return obstructedColor_; }

    /**
     * Sets the sample point color
     * @param[in ] color for the sample points (rgba, [0..1])
     */
    void setSamplePointColor(const osg::Vec4& color);

    /**
     * Gets the sample point color
     */
    const osg::Vec4& getSamplePointColor() const { return samplePointColor_; }


  public: // GeoPositionNode

    /** Return the proper library name */
    virtual const char* libraryName() const { return "simVis"; }

    /** Return the class name */
    virtual const char* className() const { return "RadialLOSNode"; }

  public: // TerrainCallback

    /** Adjusts height of node based on new tiles */
    void onTileAdded(const osgEarth::TileKey& key, osg::Node* tile, osgEarth::TerrainCallbackContext& context);

  public: // MapNodeObserver

    /** Sets the map node, used for positioning */
    virtual void setMapNode(osgEarth::MapNode* mapNode);

  protected:
    /** dtor */
    virtual ~RadialLOSNode() { }

  public:
    /** internal, updates the model of the LOS node */
    virtual void updateDataModel(
      const osgEarth::GeoExtent& extent,
      osg::Node*                 patch);

  private:
    RadialLOS           los_;
    simCore::Coordinate coord_;
    osg::Geode*         geode_;
    osg::Vec4           visibleColor_;
    osg::Vec4           obstructedColor_;
    osg::Vec4           samplePointColor_;
    osgEarth::GeoCircle bound_;
    osgEarth::optional<RadialLOS> losPrevious_;

    void refreshGeometry_();

    // called by the terrain callback when a new tile enters the graph
    void onTileAdded_(const osgEarth::TileKey& key, osg::Node* tile);

    // callback hook.
    struct TerrainCallbackHook : public osgEarth::TerrainCallback
    {
      osg::observer_ptr<RadialLOSNode> node_;
      TerrainCallbackHook(RadialLOSNode* node): node_(node) {}
      void onTileAdded(const osgEarth::TileKey& key, osg::Node* tile, osgEarth::TerrainCallbackContext&)
      {
        if (node_.valid()) node_->onTileAdded_(key, tile);
      }
    };
  };

} // namespace simVis


#endif // SIMVIS_RADIAL_LOS_NODE_H
