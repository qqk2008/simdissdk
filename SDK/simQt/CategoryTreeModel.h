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
#ifndef SIMQT_CATEGORYTREEMODEL_H
#define SIMQT_CATEGORYTREEMODEL_H

#include <map>
#include <memory>
#include <vector>
#include <QAbstractItemModel>
#include <QSortFilterProxyModel>
#include <QStyledItemDelegate>
#include "simCore/Common/Common.h"
#include "simData/CategoryData/CategoryFilter.h"
#include "simData/ObjectId.h"

class QAction;
class QFont;
class QTreeView;
namespace simData {
  class CategoryFilter;
  class DataStore;
}

namespace simQt {

class AsyncCategoryCounter;
struct CategoryCountResults;
class Settings;

/**
* Container class that keeps track of a set of pointers.  The container is indexed to
* provide O(lg n) responses to indexOf() while maintaining O(1) on access-by-index.
* The trade-off is a second internal container that maintains a list of indices.
*
* This is a template container.  Typename T can be any type that can be deleted.
*
* This class is particularly useful for Abstract Item Models that need to know things like
* the indexOf() for a particular entry.
*/
template <typename T>
class SDKQT_EXPORT IndexedPointerContainer
{
public:
  IndexedPointerContainer();
  virtual ~IndexedPointerContainer();

  /** Retrieves the item at the given index.  Not range checked.  O(1). */
  T* operator[](int index) const;
  /** Retrieves the index of the given item.  Returns -1 on not-found.  O(lg n). */
  int indexOf(const T* ptr) const;
  /** Returns the number of items in the container. */
  int size() const;
  /** Adds an item into the container.  Must be a unique item. */
  void push_back(T* item);
  /** Convenience method to delete each item, then clear(). */
  void deleteAll();

private:
  /** Vector of pointers. */
  typename std::vector<T*> vec_;
  /** Maps pointers to their index in the vector. */
  typename std::map<T*, int> itemToIndex_;
};

/// Used to sort and filter the CategoryTreeModel
class SDKQT_EXPORT CategoryProxyModel : public QSortFilterProxyModel
{
  Q_OBJECT

public:
  /// constructor passes parent to QSortFilterProxyModel
  explicit CategoryProxyModel(QObject *parent = 0);
  virtual ~CategoryProxyModel();

public slots:
  /// string to filter against
  void setFilterText(const QString& filter);
  /// Rests the filter by calling invalidateFilter
  void resetFilter();

protected:
  /// filtering function
  virtual bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const;

private:
  /// string to filter against
  QString filter_;
};

/** Single tier tree model that maintains and allows users to edit a simData::CategoryFilter. */
class SDKQT_EXPORT CategoryTreeModel : public QAbstractItemModel
{
  Q_OBJECT;
public:
  explicit CategoryTreeModel(QObject* parent = NULL);
  virtual ~CategoryTreeModel();

  /** Changes the data store, updating what categories and values are shown. */
  void setDataStore(simData::DataStore* dataStore);
  /** Retrieves the category filter.  Only call this if the Data Store has been set. */
  const simData::CategoryFilter& categoryFilter() const;
  /** Sets the settings and the key prefix for saving and loading the locked states */
  void setSettings(Settings* settings, const QString& settingsKeyPrefix);

  /** Enumeration of user roles supported by data() */
  enum {
    ROLE_SORT_STRING = Qt::UserRole,
    ROLE_EXCLUDE,
    ROLE_CATEGORY_NAME,
    ROLE_REGEXP_STRING,
    ROLE_LOCKED_STATE
  };

  // QAbstractItemModel overrides
  virtual QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
  virtual QModelIndex parent(const QModelIndex &child) const;
  virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
  virtual int columnCount(const QModelIndex &parent = QModelIndex()) const;
  virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
  virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
  virtual Qt::ItemFlags flags(const QModelIndex& index) const;
  virtual bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole);

  /// data role for obtaining names that are remapped to force "Unlisted Value" and "No Value" to the top
  static const int SortRole = Qt::UserRole + 1;

public slots:
  /** Changes the model state to match the values in the filter. */
  void setFilter(const simData::CategoryFilter& filter);
  /** Given results of a category count, updates the text for each category. */
  void processCategoryCounts(const simQt::CategoryCountResults& results);

signals:
  /** The internal filter has changed, possibly from user editing or programmatically. */
  void filterChanged(const simData::CategoryFilter& filter);
  /** The internal filter has changed from user editing. */
  void filterEdited(const simData::CategoryFilter& filter);
  /** Called when the match/exclude button changes.  Only emitted if filterChanged() is not emitted. */
  void excludeEdited(int nameInt = 0, bool excludeMode = true);

private:
  class CategoryItem;
  class ValueItem;

  /** Adds the category name into the tree structure. */
  void addName_(int nameInt);
  /** Adds the category value into the tree structure under the given name. */
  void addValue_(int nameInt, int valueInt);
  /** Remove all categories and values. */
  void clearTree_();
  /** Retrieve the CategoryItem representing the name provided. */
  CategoryItem* findNameTree_(int nameInt) const;
  /**
  * Update the locked state of the specified category if its name appears in the lockedCategories list.
  * This method should only be called on data that is updating, since it doesn't emit its own signal for a data change
  */
  void updateLockedState_(const QStringList& lockedCategories, CategoryItem& category);
  /** Emits dataChanged() signal for all child entries (non-recursive) */
  void emitChildrenDataChanged_(const QModelIndex& parent);

  /** Quick-search vector of category tree items */
  simQt::IndexedPointerContainer<CategoryItem> categories_;
  /** Maps category int values to CategoryItem pointers */
  std::map<int, CategoryItem*> categoryIntToItem_;

  /** Data store providing the name manager we depend on */
  simData::DataStore* dataStore_;
  /** Internal representation of the GUI settings in the form of a simData::CategoryFilter. */
  simData::CategoryFilter* filter_;

  /** Listens to CategoryNameManager to know when new categories and values are added. */
  class CategoryFilterListener;
  std::shared_ptr<CategoryFilterListener> listener_;

  /** Font used for the Category Name tree items */
  QFont* categoryFont_;

  /** Ptr to settings for storing locked states */
  Settings* settings_;
  /** Key for accessing the setting */
  QString settingsKey_;
};

// Typedef to CategoryTreeModel2 for backwards compatibility
#ifdef USE_DEPRECATED_SIMDISSDK_API
typedef CategoryTreeModel CategoryTreeModel2;
#endif

/**
 * Item delegate that provides custom styling for a QTreeView with a CategoryTreeModel.  This
 * delegate is required in order to get "Unlisted Value" editing working properly with
 * CategoryTreeModel.  The Unlisted Value editing is shown as an EXCLUDE flag on the category
 * itself, using a toggle switch to draw the on/off state.  Clicking on the toggle will change
 * the value in the tree model and therefore in the filter.
 *
 * Because the item delegate does not have direct access to the QTreeView on which it is
 * placed, it cannot correctly deal with clicking on expand/collapse icons.  Please listen
 * for the expandClicked() signal when using this class in order to deal with expanding
 * and collapsing trees.
 */
class SDKQT_EXPORT CategoryTreeItemDelegate : public QStyledItemDelegate
{
  Q_OBJECT;
public:
  explicit CategoryTreeItemDelegate(QObject* parent = NULL);
  virtual ~CategoryTreeItemDelegate();

  /** Overrides from QStyledItemDelegate */
  virtual void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const;
  virtual bool editorEvent(QEvent* evt, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index);

  /** Overrides from QAbstractItemDelegate */
  virtual bool helpEvent(QHelpEvent* evt, QAbstractItemView* view, const QStyleOptionViewItem& option, const QModelIndex& index);

signals:
  /** User clicked on the custom expand button and index needs to be expanded/collapsed. */
  void expandClicked(const QModelIndex& index);
  /** User clicked on the custom RegExp edit button and index needs a RegExp assigned. */
  void editRegExpClicked(const QModelIndex& index);

private:
  /** Handles paint() for category name items */
  void paintCategory_(QPainter* painter, QStyleOptionViewItem& option, const QModelIndex& index) const;
  /** Handles paint() for value items */
  void paintValue_(QPainter* painter, QStyleOptionViewItem& option, const QModelIndex& index) const;

  /** Handles editorEvent() for category name items */
  bool categoryEvent_(QEvent* evt, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index);
  /** Handles editorEvent() for value items. */
  bool valueEvent_(QEvent* evt, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index);

  /** Sub-elements vary depending on the type of index to draw. */
  enum SubElement
  {
    SE_NONE = 0,
    SE_BACKGROUND,
    SE_CHECKBOX,
    SE_BRANCH,
    SE_TEXT,
    SE_EXCLUDE_TOGGLE,
    SE_REGEXP_BUTTON
  };
  /** Contains the rectangles for all sub-elements for an index. */
  struct ChildRects;
  /** Calculate the drawn rectangle areas for each sub-element of a given index. */
  void calculateRects_(const QStyleOptionViewItem& option, const QModelIndex& index, ChildRects& rects) const;
  /** Determine which sub-element, if any, was hit in a mouse click.  See QMouseEvent::pos(). */
  SubElement hit_(const QPoint& pos, const QStyleOptionViewItem& option, const QModelIndex& index) const;

  /** Keeps track of the QModelIndex being clicked. */
  QModelIndex clickedIndex_;
  /** Sub-element being clicked */
  SubElement clickedElement_;
};

/**
 * Widget that includes a QTreeView with a Category Tree Model and a Search Filter
 * widget that will display a given category filter.  This is an easy-to-use wrapper
 * around the CategoryTreeModel class that provides a view widget and search field.
 */
class SDKQT_EXPORT CategoryFilterWidget : public QWidget
{
  Q_OBJECT;
  Q_PROPERTY(bool showEntityCount READ showEntityCount WRITE setShowEntityCount);

public:
  explicit CategoryFilterWidget(QWidget* parent = 0);
  virtual ~CategoryFilterWidget();

  /** Sets the data store, updating the category tree based on changes to that data store. */
  void setDataStore(simData::DataStore* dataStore);
  /** Retrieves the category filter.  Only call this if the Data Store has been set. */
  const simData::CategoryFilter& categoryFilter() const;
  /** Sets the settings and the key prefix for saving and loading the locked states */
  void setSettings(Settings* settings, const QString& settingsKeyPrefix);

  /** Returns true if the entity count should be shown next to values. */
  bool showEntityCount() const;
  /** Changes whether entity count is shown next to category values. */
  void setShowEntityCount(bool show);

public slots:
  /** Changes the model state to match the values in the filter. */
  void setFilter(const simData::CategoryFilter& filter);
  /** Updates the (#) count next to category values with the given category value counts. */
  void processCategoryCounts(const simQt::CategoryCountResults& results);
  /** Shows a GUI for editing the regular expression of a given index */
  void showRegExpEditGui_(const QModelIndex& index);

signals:
  /** The internal filter has changed, possibly from user editing or programmatically. */
  void filterChanged(const simData::CategoryFilter& filter);
  /** The internal filter has changed from user editing. */
  void filterEdited(const simData::CategoryFilter& filter);

private slots:
  /** Expand the given index from the proxy if filtering */
  void expandDueToProxy_(const QModelIndex& parentIndex, int to, int from);
  /** Conditionally expand tree after filter edited. */
  void expandAfterFilterEdited_(const QString& filterText);

  /** Called by delegate to expand an item */
  void toggleExpanded_(const QModelIndex& proxyIndex);
  /** Right click occurred on the QTreeView, point relative to QTreeView */
  void showContextMenu_(const QPoint& point);

  /** Reset the active filter, clearing all values */
  void resetFilter_();
  /** Sets the regular expression on the item saved from showContextMenu_ */
  void setRegularExpression_();
  /** Clears the regular expression on the item saved from showContextMenu_ */
  void clearRegularExpression_();
  /** Locks the current category saved from the showContextMenu_ */
  void toggleLockCategory_();
  /** Expands all unlocked categories */
  void expandUnlockedCategories_();
  /** Start a recount of the category values if countDirty_ is true */
  void recountCategories_();

private:
  class DataStoreListener;

  /** The tree */
  QTreeView* treeView_;
  /** Hold the category data */
  simQt::CategoryTreeModel* treeModel_;
  /** Provides sorting and filtering */
  simQt::CategoryProxyModel* proxy_;
  /** If true the category values are filtered; used to conditionally expand tree. */
  bool activeFiltering_;
  /** If true the category values show a (#) count after them. */
  bool showEntityCount_;
  /** Counter object that provides values for entity counting. */
  AsyncCategoryCounter* counter_;
  /** Action used for setting regular expressions */
  QAction* setRegExpAction_;
  /** Action used for clearing regular expressions */
  QAction* clearRegExpAction_;
  /** Action used for toggling the lock state of a category */
  QAction* toggleLockCategoryAction_;
  /** Listener for datastore entity events */
  std::shared_ptr<DataStoreListener> dsListener_;
  /** If true then the category counts need to be redone */
  bool countDirty_;
};

// Typedef to CategoryFilterWidget2 for backwards compatibility
#ifdef USE_DEPRECATED_SIMDISSDK_API
typedef CategoryFilterWidget CategoryFilterWidget2;
#endif

}

#endif /* SIMQT_CATEGORYTREEMODEL_H */
