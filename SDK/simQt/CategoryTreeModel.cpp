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
#include <QAbstractItemView>
#include <QAction>
#include <QApplication>
#include <QColor>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFont>
#include <QLabel>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QTimer>
#include <QToolTip>
#include <QTreeView>
#include <QVBoxLayout>
#include "simData/CategoryData/CategoryFilter.h"
#include "simData/CategoryData/CategoryNameManager.h"
#include "simData/DataStore.h"
#include "simQt/QtFormatting.h"
#include "simQt/CategoryFilterCounter.h"
#include "simQt/EntityFilterLineEdit.h"
#include "simQt/RegExpImpl.h"
#include "simQt/SearchLineEdit.h"
#include "simQt/Settings.h"
#include "simQt/CategoryTreeModel.h"

namespace simQt {

/** Lighter than lightGray, matches QPalette::Midlight */
static const QColor MIDLIGHT_BG_COLOR(227, 227, 227);
/** Breadcrumb's default fill color, used here for background brush on filter items that contribute to filter. */
static const QColor CONTRIBUTING_BG_COLOR(195, 225, 240); // Light gray with a hint of blue
/** Locked settings keys */
static const QString LOCKED_SETTING = "LockedCategories";
/** Locked settings meta data to define it as private */
static const simQt::Settings::MetaData LOCKED_SETTING_METADATA(Settings::STRING_LIST, "", "", Settings::PRIVATE);


/////////////////////////////////////////////////////////////////////////

template <typename T>
IndexedPointerContainer<T>::IndexedPointerContainer()
{
}

template <typename T>
IndexedPointerContainer<T>::~IndexedPointerContainer()
{
  deleteAll();
}

template <typename T>
T* IndexedPointerContainer<T>::operator[](int index) const
{
  return vec_[index];
}

template <typename T>
int IndexedPointerContainer<T>::indexOf(const T* item) const
{
  // Use a const-cast to help find() to use right signature
  const auto i = itemToIndex_.find(const_cast<T*>(item));
  return (i == itemToIndex_.end() ? -1 : i->second);
}

template <typename T>
int IndexedPointerContainer<T>::size() const
{
  return static_cast<int>(vec_.size());
}

template <typename T>
void IndexedPointerContainer<T>::push_back(T* item)
{
  // Don't add the same item twice
  assert(itemToIndex_.find(item) == itemToIndex_.end());
  const int index = size();
  vec_.push_back(item);
  itemToIndex_[item] = index;
}

template <typename T>
void IndexedPointerContainer<T>::deleteAll()
{
  for (auto i = vec_.begin(); i != vec_.end(); ++i)
    delete *i;
  vec_.clear();
  itemToIndex_.clear();
}

/////////////////////////////////////////////////////////////////////////

/**
* Base class for an item in the composite pattern of Category Tree Item / Value Tree Item.
* Note that child trees to this class are owned by this class (in the IndexedPointerContainer).
*/
class TreeItem
{
public:
  TreeItem();
  virtual ~TreeItem();

  /** Forward from QAbstractItemModel::data() */
  virtual QVariant data(int role) const = 0;
  /** Forward from QAbstractItemModel::flags() */
  virtual Qt::ItemFlags flags() const = 0;
  /** Returns true if the GUI changed; sets filterChanged if filter edited. */
  virtual bool setData(const QVariant& value, int role, simData::CategoryFilter& filter, bool& filterChanged) = 0;

  /** Retrieves the category name this tree item is associated with */
  virtual QString categoryName() const = 0;
  /** Returns the category name integer value for this item or its parent */
  virtual int nameInt() const = 0;
  /** Returns true if the UNLISTED VALUE item is checked (i.e. if we are in EXCLUDE mode) */
  virtual bool isUnlistedValueChecked() const = 0;
  /** Returns true if the tree item's category is influenced by a regular expression */
  virtual bool isRegExpApplied() const = 0;

  ///@{ Composite Tree Management Methods
  TreeItem* parent() const;
  int rowInParent() const;
  int indexOf(const TreeItem* child) const;
  TreeItem* child(int index) const;
  int childCount() const;
  void addChild(TreeItem* item);
  ///@}

private:
  TreeItem* parent_;
  simQt::IndexedPointerContainer<TreeItem> children_;
};

/////////////////////////////////////////////////////////////////////////

/** Represents a group node in tree, showing a category name and containing children values. */
class CategoryTreeModel::CategoryItem : public TreeItem
{
public:
  CategoryItem(const simData::CategoryNameManager& nameManager, int nameInt);

  /** TreeItem Overrides */
  virtual Qt::ItemFlags flags() const;
  virtual QVariant data(int role) const;
  virtual bool setData(const QVariant& value, int role, simData::CategoryFilter& filter, bool& filterChanged);
  virtual QString categoryName() const;
  virtual int nameInt() const;
  virtual bool isUnlistedValueChecked() const;
  virtual bool isRegExpApplied() const;

  /** Recalculates the "contributes to filter" flag, returning true if it changes (like setData()) */
  bool recalcContributionTo(const simData::CategoryFilter& filter);

  /** Changes the font to use. */
  void setFont(QFont* font);
  /** Sets the state of the GUI to match the state of the filter. Returns 0 if nothing changed. */
  int updateTo(const simData::CategoryFilter& filter);

  /** Sets the ID counts for each value under this category name tree, returning true if there is a change. */
  bool updateCounts(const std::map<int, size_t>& valueToCountMap) const;

private:
  /** Checks and unchecks children based on whether they match the filter, returning true if any checks change. */
  bool setChildChecks_(const simData::RegExpFilter* reFilter);

  /** Changes the filter to match the check state of the Value Item. */
  void updateFilter_(const ValueItem& valueItem, simData::CategoryFilter& filter) const;
  /** Change the value item to match the state of the checks structure (filter).  Returns 0 on no change. */
  int updateValueItem_(ValueItem& valueItem, const simData::CategoryFilter::ValuesCheck& checks) const;

  /** setData() variant that handles the ROLE_EXCLUDE role */
  bool setExcludeData_(const QVariant& value, simData::CategoryFilter& filter, bool& filterChanged);
  /** setData() variant that handles ROLE_REGEXP_STRING role */
  bool setRegExpStringData_(const QVariant& value, simData::CategoryFilter& filter, bool& filterChanged);

  /** String representation of NAME. */
  QString categoryName_;
  /** Integer representation of NAME. */
  int nameInt_;
  /** Cache the state of the UNLISTED VALUE.  When TRUE, we're in EXCLUDE mode */
  bool unlistedValue_;
  /** Category's Regular Expression string value */
  QString regExpString_;
  /** Set to true if this category contributes to the filter. */
  bool contributesToFilter_;
  /** Font to use for FontRole (not owned) */
  QFont* font_;
  /** Tracks whether this category item is locked */
  bool locked_;
};

/////////////////////////////////////////////////////////////////////////

/** Represents a leaf node in tree, showing a category value. */
class CategoryTreeModel::ValueItem : public TreeItem
{
public:
  ValueItem(const simData::CategoryNameManager& nameManager, int nameInt, int valueInt);

  /** TreeItem Overrides */
  virtual Qt::ItemFlags flags() const;
  virtual QVariant data(int role) const;
  virtual bool setData(const QVariant& value, int role, simData::CategoryFilter& filter, bool& filterChanged);
  virtual QString categoryName() const;
  virtual int nameInt() const;
  virtual bool isUnlistedValueChecked() const;
  virtual bool isRegExpApplied() const;

  /** Returns the value integer for this item */
  int valueInt() const;
  /** Returns the value string for this item; for NO_CATEGORY_VALUE_AT_TIME, empty string is returned. */
  QString valueString() const;

  /**
  * Changes the GUI state of whether this item is checked.  This does not match 1-for-1
  * with the filter state, and does not directly update any CategoryFilter instance.
  */
  void setChecked(bool value);
  /** Returns true if the GUI state is such that this item is checked. */
  bool isChecked() const;

  /** Sets the number of entities that match this value.  Use -1 to reset. */
  void setNumMatches(int numMatches);
  /** Returns number entities that match this particular value in the given filter. */
  int numMatches() const;

private:
  /** setData() that handles Qt::CheckStateRole.  Returns true if GUI state changes, and sets filterChanged if filter changes. */
  bool setCheckStateData_(const QVariant& value, simData::CategoryFilter& filter, bool& filterChanged);

  int nameInt_;
  int valueInt_;
  int numMatches_;
  Qt::CheckState checked_;
  QString valueString_;
};

/////////////////////////////////////////////////////////////////////////

TreeItem::TreeItem()
  : parent_(NULL)
{
}

TreeItem::~TreeItem()
{
  children_.deleteAll();
}

TreeItem* TreeItem::parent() const
{
  return parent_;
}

int TreeItem::rowInParent() const
{
  if (parent_ == NULL)
  {
    // Caller is getting an invalid value
    assert(0);
    return -1;
  }
  return parent_->indexOf(this);
}

int TreeItem::indexOf(const TreeItem* child) const
{
  return children_.indexOf(child);
}

TreeItem* TreeItem::child(int index) const
{
  return children_[index];
}

int TreeItem::childCount() const
{
  return children_.size();
}

void TreeItem::addChild(TreeItem* item)
{
  // Assertion failure means developer is doing something weird.
  assert(item != NULL);
  // Assertion failure means that item is inserted more than once.
  assert(item->parent() == NULL);

  // Set the parent and save the item in our children vector.
  item->parent_ = this;
  children_.push_back(item);
}

/////////////////////////////////////////////////////////////////////////

CategoryTreeModel::CategoryItem::CategoryItem(const simData::CategoryNameManager& nameManager, int nameInt)
  : categoryName_(QString::fromStdString(nameManager.nameIntToString(nameInt))),
    nameInt_(nameInt),
    unlistedValue_(false),
    contributesToFilter_(false),
    font_(NULL),
    locked_(false)
{
}

bool CategoryTreeModel::CategoryItem::isUnlistedValueChecked() const
{
  return unlistedValue_;
}

bool CategoryTreeModel::CategoryItem::isRegExpApplied() const
{
  return !regExpString_.isEmpty();
}

int CategoryTreeModel::CategoryItem::nameInt() const
{
  return nameInt_;
}

QString CategoryTreeModel::CategoryItem::categoryName() const
{
  return categoryName_;
}

Qt::ItemFlags CategoryTreeModel::CategoryItem::flags() const
{
  return Qt::ItemIsEnabled;
}

QVariant CategoryTreeModel::CategoryItem::data(int role) const
{
  switch (role)
  {
  case Qt::DisplayRole:
  case Qt::EditRole:
  case ROLE_SORT_STRING:
  case ROLE_CATEGORY_NAME:
    return categoryName_;
  case ROLE_EXCLUDE:
    return unlistedValue_;
  case ROLE_REGEXP_STRING:
    return regExpString_;
  case ROLE_LOCKED_STATE:
    return locked_;
  case Qt::BackgroundRole:
    if (contributesToFilter_)
      return CONTRIBUTING_BG_COLOR;
    return MIDLIGHT_BG_COLOR;
  case Qt::FontRole:
    if (font_)
      return *font_;
    break;
  default:
    break;
  }
  return QVariant();
}

bool CategoryTreeModel::CategoryItem::setData(const QVariant& value, int role, simData::CategoryFilter& filter, bool& filterChanged)
{
  if (role == ROLE_EXCLUDE)
    return setExcludeData_(value, filter, filterChanged);
  else if (role == ROLE_REGEXP_STRING)
    return setRegExpStringData_(value, filter, filterChanged);
  else if (role == ROLE_LOCKED_STATE && locked_ != value.toBool())
  {
    locked_ = value.toBool();
    filterChanged = true;
    return true;
  }
  filterChanged = false;
  return false;
}

bool CategoryTreeModel::CategoryItem::setExcludeData_(const QVariant& value, simData::CategoryFilter& filter, bool& filterChanged)
{
  filterChanged = false;
  // If value does not change, or if disabled, then return early
  if (value.toBool() == unlistedValue_ || !flags().testFlag(Qt::ItemIsEnabled))
    return false;

  // Update the value
  unlistedValue_ = value.toBool();

  // If the filter does not include our category, then we do nothing RE: filter
  auto values = filter.getCategoryFilter();
  if (values.find(nameInt_) == values.end())
    return true; // True, update our GUI -- but note that the filter did not change

  // Remove the whole name from the filter, then build it from scratch from GUI
  filterChanged = true;
  filter.removeName(nameInt_);
  filter.setValue(nameInt_, simData::CategoryNameManager::UNLISTED_CATEGORY_VALUE, unlistedValue_);
  const int count = childCount();
  for (int k = 0; k < count; ++k)
    updateFilter_(*static_cast<ValueItem*>(child(k)), filter);
  filter.simplify(nameInt_);

  // Update the flag for contributing to the filter
  recalcContributionTo(filter);
  return true;
}

bool CategoryTreeModel::CategoryItem::setRegExpStringData_(const QVariant& value, simData::CategoryFilter& filter, bool& filterChanged)
{
  // Check for easy no-op
  filterChanged = false;
  if (value.toString() == regExpString_)
    return false;

  // Update the value
  regExpString_ = value.toString();
  filterChanged = true;

  // Create/set the regular expression
  simData::RegExpFilterPtr newRegExpObject;
  if (!regExpString_.isEmpty())
  {
    // The factory could/should be passed in for maximum flexibility
    simQt::RegExpFilterFactoryImpl reFactory;
    newRegExpObject = reFactory.createRegExpFilter(regExpString_.toStdString());
  }

  // Set the RegExp, simplify, and update the internal state
  filter.setCategoryRegExp(nameInt_, newRegExpObject);
  filter.simplify(nameInt_);
  recalcContributionTo(filter);
  setChildChecks_(newRegExpObject.get());
  return true;
}

bool CategoryTreeModel::CategoryItem::recalcContributionTo(const simData::CategoryFilter& filter)
{
  // First check the regular expression.  If there's a regexp, then this category definitely contributes
  const bool newValue = filter.nameContributesToFilter(nameInt_);
  if (newValue == contributesToFilter_)
    return false;
  contributesToFilter_ = newValue;
  return true;
}

void CategoryTreeModel::CategoryItem::setFont(QFont* font)
{
  font_ = font;
}

bool CategoryTreeModel::CategoryItem::setChildChecks_(const simData::RegExpFilter* reFilter)
{
  bool hasChange = false;
  const int count = childCount();
  for (int k = 0; k < count; ++k)
  {
    // Test the EditRole, which is used because it omits the # count (e.g. "Friendly (1)")
    ValueItem* valueItem = static_cast<ValueItem*>(child(k));
    const bool matches = reFilter != NULL && reFilter->match(valueItem->valueString().toStdString());
    if (matches != valueItem->isChecked())
    {
      valueItem->setChecked(matches);
      hasChange = true;
    }
  }
  return hasChange;
}

int CategoryTreeModel::CategoryItem::updateTo(const simData::CategoryFilter& filter)
{
  // Update the category if it has a RegExp
  const QString oldRegExp = regExpString_;
  const auto* regExpObject = filter.getRegExp(nameInt_);
  regExpString_ = (regExpObject != NULL ? QString::fromStdString(filter.getRegExpPattern(nameInt_)) : "");
  // If the RegExp string is different, we definitely have some sort of change
  bool hasChange = (regExpString_ != oldRegExp);

  // Case 1: Regular Expression is not empty.  Check and uncheck values as needed
  if (!regExpString_.isEmpty())
  {
    // Synchronize the checks of the children
    if (setChildChecks_(regExpObject))
      hasChange = true;
    return hasChange ? 1 : 0;
  }

  // No RegExp -- pull out the category checks
  simData::CategoryFilter::ValuesCheck checks;
  filter.getValues(nameInt_, checks);

  // Case 2: Filter doesn't have this category.  Uncheck all children
  if (checks.empty())
  {
    const int count = childCount();
    for (int k = 0; k < count; ++k)
    {
      ValueItem* valueItem = static_cast<ValueItem*>(child(k));
      if (valueItem->isChecked())
      {
        valueItem->setChecked(false);
        hasChange = true;
      }
    }

    // Fix filter on/off
    if (recalcContributionTo(filter))
      hasChange = true;
    return hasChange ? 1 : 0;
  }

  // Case 3: We are in the filter, so our unlistedValueBool matters
  auto i = checks.find(simData::CategoryNameManager::UNLISTED_CATEGORY_VALUE);
  if (i != checks.end())
  {
    // Unlisted value present means it must be on
    assert(i->second);
  }

  // Detect change in Unlisted Value state
  const bool newUnlistedValue = (i != checks.end() && i->second);
  if (unlistedValue_ != newUnlistedValue)
    hasChange = true;
  unlistedValue_ = newUnlistedValue;

  // Iterate through children and make sure the state matches
  const int count = childCount();
  for (int k = 0; k < count; ++k)
  {
    if (0 != updateValueItem_(*static_cast<ValueItem*>(child(k)), checks))
      hasChange = true;
  }

  // Update the flag for contributing to the filter
  if (recalcContributionTo(filter))
    hasChange = true;

  return hasChange ? 1 : 0;
}

void CategoryTreeModel::CategoryItem::updateFilter_(const ValueItem& valueItem, simData::CategoryFilter& filter) const
{
  const bool filterValue = (valueItem.isChecked() != unlistedValue_);
  // NO_VALUE is a special case
  if (valueItem.valueInt() == simData::CategoryNameManager::NO_CATEGORY_VALUE_AT_TIME)
  {
    if (filterValue)
      filter.setValue(nameInt_, valueItem.valueInt(), true);
  }
  else
  {
    if (filterValue != unlistedValue_)
      filter.setValue(nameInt_, valueItem.valueInt(), filterValue);
  }
}

int CategoryTreeModel::CategoryItem::updateValueItem_(ValueItem& valueItem, const simData::CategoryFilter::ValuesCheck& checks) const
{
  // NO VALUE is a special case unfortunately
  const auto i = checks.find(valueItem.valueInt());
  bool nextCheckedState = false;
  if (valueItem.valueInt() == simData::CategoryNameManager::NO_CATEGORY_VALUE_AT_TIME)
  {
    // Item is a NO-VALUE item.  This does not follow the rules of "unlisted value"
    // in CategoryFilter class, so it's a special case, because we DO want to follow
    // logical rules for the end user here in this GUI.
    const bool showingNoValue = (i != checks.end() && i->second);
    // If unlisted value is false, then we show the NO VALUE as checked if its check
    // is present and on.  If unlisted value is true, then we invert the display
    // so that No-Value swaps into No-No-Value, or Has-Value for short.  This all
    // simplifies into the expression "setChecked(unlisted != showing)".
    nextCheckedState = (unlistedValue_ != showingNoValue);
  }
  else if (unlistedValue_)
  {
    // "Harder" case.  Unlisted Values are checked, so GUI is showing "omit" or "not"
    // states.  If it's checked, then we're explicitly omitting that value.  So the
    // only way to omit is if there is an entry in the checks, and it's set false.
    nextCheckedState = (i != checks.end() && !i->second);
  }
  else
  {
    // "Simple" case.  Unlisted Values are unchecked, so we're matching ONLY items
    // that are in the filter, that are checked.  So to be checked in the GUI, the
    // value must have a checkmark
    nextCheckedState = (i != checks.end() && i->second);
  }

  if (nextCheckedState == valueItem.isChecked())
    return 0;
  valueItem.setChecked(nextCheckedState);
  return 1;
}

bool CategoryTreeModel::CategoryItem::updateCounts(const std::map<int, size_t>& valueToCountMap) const
{
  const int numValues = childCount();
  bool haveChange = false;
  for (int k = 0; k < numValues; ++k)
  {
    ValueItem* valueItem = dynamic_cast<ValueItem*>(child(k));
    // All children should be ValueItems
    assert(valueItem);
    if (!valueItem)
      continue;

    // It's entirely possible (through async methods) that the incoming value count map is not
    // up to date.  This can occur if a count starts and more categories get added before the
    // count finishes, and is common.
    auto i = valueToCountMap.find(valueItem->valueInt());
    int nextMatch = -1;
    if (i != valueToCountMap.end())
      nextMatch = static_cast<int>(i->second);

    // Set the number of matches and record a change
    if (valueItem->numMatches() != nextMatch)
    {
      valueItem->setNumMatches(nextMatch);
      haveChange = true;
    }
  }

  return haveChange;
}

/////////////////////////////////////////////////////////////////////////

CategoryTreeModel::ValueItem::ValueItem(const simData::CategoryNameManager& nameManager, int nameInt, int valueInt)
  : nameInt_(nameInt),
    valueInt_(valueInt),
    numMatches_(-1),
    checked_(Qt::Unchecked),
    valueString_(QString::fromStdString(nameManager.valueIntToString(valueInt)))
{
}

bool CategoryTreeModel::ValueItem::isUnlistedValueChecked() const
{
  // Assertion failure means we have orphan value items
  assert(parent());
  if (!parent())
    return false;
  return parent()->isUnlistedValueChecked();
}

bool CategoryTreeModel::ValueItem::isRegExpApplied() const
{
  // Assertion failure means we have orphan value items
  assert(parent());
  if (!parent())
    return false;
  return parent()->isRegExpApplied();
}

int CategoryTreeModel::ValueItem::nameInt() const
{
  return nameInt_;
}

QString CategoryTreeModel::ValueItem::categoryName() const
{
  // Assertion failure means we have orphan value items
  assert(parent());
  if (!parent())
    return "";
  return parent()->data(ROLE_CATEGORY_NAME).toString();
}

int CategoryTreeModel::ValueItem::valueInt() const
{
  return valueInt_;
}

QString CategoryTreeModel::ValueItem::valueString() const
{
  // "No Value" should return empty string here, not user-facing string
  if (valueInt_ == simData::CategoryNameManager::NO_CATEGORY_VALUE_AT_TIME)
    return "";
  return valueString_;
}

Qt::ItemFlags CategoryTreeModel::ValueItem::flags() const
{
  if (isRegExpApplied())
    return Qt::NoItemFlags;
  return Qt::ItemIsEnabled | Qt::ItemIsUserCheckable;
}

QVariant CategoryTreeModel::ValueItem::data(int role) const
{
  switch (role)
  {
  case Qt::DisplayRole:
  case Qt::EditRole:
  {
    QString returnString;
    if (!isUnlistedValueChecked())
      returnString = valueString_;
    else if (valueInt_ == simData::CategoryNameManager::NO_CATEGORY_VALUE_AT_TIME)
      returnString = tr("Has Value");
    else
      returnString = tr("Not %1").arg(valueString_);
    // Append the numeric count if specified -- only if in include mode, and NOT in exclude mode
    if (numMatches_ >= 0 && !isUnlistedValueChecked())
      returnString = tr("%1 (%2)").arg(returnString).arg(numMatches_);
    return returnString;
  }

  case Qt::CheckStateRole:
    return checked_;

  case ROLE_SORT_STRING:
    if (valueInt_ == simData::CategoryNameManager::NO_CATEGORY_VALUE_AT_TIME)
      return QString("");
    return data(Qt::DisplayRole);

  case ROLE_EXCLUDE:
    return isUnlistedValueChecked();

  case ROLE_CATEGORY_NAME:
    return categoryName();

  case ROLE_REGEXP_STRING:
    // Parent node holds the RegExp string
    if (parent())
      return parent()->data(ROLE_REGEXP_STRING);
    break;

  case ROLE_LOCKED_STATE:
    // Parent node holds the lock state
    if (parent())
      return parent()->data(ROLE_LOCKED_STATE);
    break;

  default:
    break;
  }
  return QVariant();
}

bool CategoryTreeModel::ValueItem::setData(const QVariant& value, int role, simData::CategoryFilter& filter, bool& filterChanged)
{
  // Internally handle check/uncheck value.  For ROLE_REGEXP and ROLE_LOCKED_STATE, rely on category parent
  if (role == Qt::CheckStateRole)
    return setCheckStateData_(value, filter, filterChanged);
  else if (role == ROLE_REGEXP_STRING && parent() != NULL)
    return parent()->setData(value, role, filter, filterChanged);
  else if (role == ROLE_LOCKED_STATE && parent() != NULL)
    return parent()->setData(value, role, filter, filterChanged);
  filterChanged = false;
  return false;
}

bool CategoryTreeModel::ValueItem::setCheckStateData_(const QVariant& value, simData::CategoryFilter& filter, bool& filterChanged)
{
  filterChanged = false;

  // If the edit sets us to same state, or disabled, then return early
  const Qt::CheckState newChecked = static_cast<Qt::CheckState>(value.toInt());
  if (newChecked == checked_ || !flags().testFlag(Qt::ItemIsEnabled))
    return false;

  // Figure out how to translate the GUI state into the filter value
  checked_ = newChecked;
  const bool unlistedValue = isUnlistedValueChecked();
  const bool checkedBool = (checked_ == Qt::Checked);
  const bool filterValue = (unlistedValue != checkedBool);

  // Change the value in the filter.  NO VALUE is a special case
  if (valueInt_ == simData::CategoryNameManager::NO_CATEGORY_VALUE_AT_TIME)
  {
    // If the filter value is off, then remove it from the filter; it's always off by default
    if (!filterValue)
      filter.removeValue(nameInt_, valueInt_);
    else
      filter.setValue(nameInt_, valueInt_, true);
  }
  else
  {
    // Remove items that match unlisted value.  Add items that do not.
    if (filterValue == unlistedValue)
      filter.removeValue(nameInt_, valueInt_);
    else
    {
      // If the filter was previously empty and we're setting a value, we need to
      // make sure that the "No Value" check is correctly set in some cases.
      if (!filterValue && unlistedValue)
      {
        simData::CategoryFilter::ValuesCheck checks;
        filter.getValues(nameInt_, checks);
        if (checks.empty())
          filter.setValue(nameInt_, simData::CategoryNameManager::NO_CATEGORY_VALUE_AT_TIME, true);
      }

      filter.setValue(nameInt_, valueInt_, filterValue);
    }
  }

  // Ensure UNLISTED VALUE is set correctly.
  if (unlistedValue)
    filter.setValue(nameInt_, simData::CategoryNameManager::UNLISTED_CATEGORY_VALUE, true);
  else
    filter.removeValue(nameInt_, simData::CategoryNameManager::UNLISTED_CATEGORY_VALUE);
  // Make sure the filter is simplified
  filter.simplify(nameInt_);

  // Update the parent too, which fixes the GUI for whether it contributes
  CategoryItem* parentTree = dynamic_cast<CategoryItem*>(parent());
  parentTree->recalcContributionTo(filter);

  filterChanged = true;
  return true;
}

void CategoryTreeModel::ValueItem::setChecked(bool value)
{
  checked_ = (value ? Qt::Checked : Qt::Unchecked);
}

bool CategoryTreeModel::ValueItem::isChecked() const
{
  return checked_ == Qt::Checked;
}

void CategoryTreeModel::ValueItem::setNumMatches(int matches)
{
  numMatches_ = matches;
}

int CategoryTreeModel::ValueItem::numMatches() const
{
  return numMatches_;
}

/////////////////////////////////////////////////////////////////////////

/// Monitors for category data changes, calling methods in CategoryTreeModel.
class CategoryTreeModel::CategoryFilterListener : public simData::CategoryNameManager::Listener
{
public:
  /// Constructor
  explicit CategoryFilterListener(CategoryTreeModel& parent)
    : parent_(parent)
  {
  }

  virtual ~CategoryFilterListener()
  {
  }

  /// Invoked when a new category is added
  virtual void onAddCategory(int categoryIndex)
  {
    parent_.addName_(categoryIndex);
  }

  /// Invoked when a new value is added to a category
  virtual void onAddValue(int categoryIndex, int valueIndex)
  {
    parent_.addValue_(categoryIndex, valueIndex);
  }

  /// Invoked when all data is cleared
  virtual void onClear()
  {
    parent_.clearTree_();
  }

  /// Invoked when all listeners have received onClear()
  virtual void doneClearing()
  {
    // noop
  }

private:
  CategoryTreeModel& parent_;
};

/////////////////////////////////////////////////////////////////////////

CategoryProxyModel::CategoryProxyModel(QObject *parent)
  : QSortFilterProxyModel(parent)
{
}

CategoryProxyModel::~CategoryProxyModel()
{
}

void CategoryProxyModel::resetFilter()
{
  invalidateFilter();
}

void CategoryProxyModel::setFilterText(const QString& filter)
{
  if (filter_ == filter)
    return;

  filter_ = filter;
  invalidateFilter();
}

bool CategoryProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
  if (filter_.isEmpty())
    return true;

  const QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);
  const QString itemText = index.data(Qt::DisplayRole).toString();

  // include items that pass the filter
  if (itemText.contains(filter_, Qt::CaseInsensitive))
    return true;

  // include items whose parent passes the filter, but not if parent is root "All Categories" item
  if (sourceParent.isValid())
  {
    const QString parentText = sourceParent.data(Qt::DisplayRole).toString();

    if (parentText.contains(filter_, Qt::CaseInsensitive))
        return true;
  }

  // include items with any children that pass the filter
  const int numChildren = sourceModel()->rowCount(index);
  for (int ii = 0; ii < numChildren; ++ii)
  {
    const QModelIndex childIndex = sourceModel()->index(ii, 0, index);
    // Assertion failure means rowCount() was wrong
    assert(childIndex.isValid());
    const QString childText = childIndex.data(Qt::DisplayRole).toString();
    if (childText.contains(filter_, Qt::CaseInsensitive))
      return true;
  }
  return false;
}

/////////////////////////////////////////////////////////////////////////

CategoryTreeModel::CategoryTreeModel(QObject* parent)
  : QAbstractItemModel(parent),
    dataStore_(NULL),
    filter_(new simData::CategoryFilter(NULL)),
    categoryFont_(new QFont),
    settings_(NULL)
{
  listener_.reset(new CategoryFilterListener(*this));

  // Increase the point size on the category
  categoryFont_->setPointSize(categoryFont_->pointSize() + 4);
  categoryFont_->setBold(true);
}

CategoryTreeModel::~CategoryTreeModel()
{
  categories_.deleteAll();
  categoryIntToItem_.clear();
  delete categoryFont_;
  categoryFont_ = NULL;
  delete filter_;
  filter_ = NULL;
  if (dataStore_)
    dataStore_->categoryNameManager().removeListener(listener_);
}

QModelIndex CategoryTreeModel::index(int row, int column, const QModelIndex &parent) const
{
  if (!hasIndex(row, column, parent))
    return QModelIndex();
  // Category items have no parent in the model
  if (!parent.isValid())
    return createIndex(row, column, categories_[row]);
  // Has a parent: must be a value item
  TreeItem* parentItem = static_cast<TreeItem*>(parent.internalPointer());
  // Item was not made correctly, check index()
  assert(parentItem != NULL);
  return createIndex(row, column, parentItem->child(row));
}

QModelIndex CategoryTreeModel::parent(const QModelIndex &child) const
{
  if (!child.isValid() || !child.internalPointer())
    return QModelIndex();

  // Child could be a category (no parent) or a value (category parent)
  const TreeItem* childItem = static_cast<TreeItem*>(child.internalPointer());
  TreeItem* parentItem = childItem->parent();
  if (parentItem == NULL) // child is a category; no parent
    return QModelIndex();
  return createIndex(categories_.indexOf(static_cast<CategoryItem*>(parentItem)), 0, parentItem);
}

int CategoryTreeModel::rowCount(const QModelIndex &parent) const
{
  if (parent.isValid())
  {
    if (parent.column() != 0)
      return 0;
    TreeItem* parentItem = static_cast<TreeItem*>(parent.internalPointer());
    return (parentItem == NULL) ? 0 : parentItem->childCount();
  }
  return categories_.size();
}

int CategoryTreeModel::columnCount(const QModelIndex &parent) const
{
  return 1;
}

QVariant CategoryTreeModel::data(const QModelIndex &index, int role) const
{
  if (!index.isValid() || !index.internalPointer())
    return QVariant();
  const TreeItem* treeItem = static_cast<TreeItem*>(index.internalPointer());
  return treeItem->data(role);
}

QVariant CategoryTreeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
  if ((orientation == Qt::Horizontal) && (role == Qt::DisplayRole))
  {
    if (section == 0)
      return tr("Category");

    // A column was added and this section was not updated
    assert(0);
    return QVariant();
  }

  // Isn't the bar across the top -- fall back to whatever QAIM does
  return QAbstractItemModel::headerData(section, orientation, role);
}

Qt::ItemFlags CategoryTreeModel::flags(const QModelIndex& index) const
{
  if (!index.isValid() || !index.internalPointer())
    return Qt::NoItemFlags;
  TreeItem* item = static_cast<TreeItem*>(index.internalPointer());
  return item->flags();
}

bool CategoryTreeModel::setData(const QModelIndex& idx, const QVariant& value, int role)
{
  // Ensure we have a valid index with a valid TreeItem pointer
  if (!idx.isValid() || !idx.internalPointer())
    return QAbstractItemModel::setData(idx, value, role);

  // NULL filter means the tree should be empty, so we shouldn't get setData()...
  TreeItem* item = static_cast<TreeItem*>(idx.internalPointer());
  assert(filter_ && item);
  bool wasEdited = false;
  const bool rv = item->setData(value, role, *filter_, wasEdited);

  // update locked setting for this category if it is a category item and this is a locked state update
  if (settings_ && item->childCount() > 0 && role == ROLE_LOCKED_STATE)
  {
    QStringList lockedCategories = settings_->value(settingsKey_, LOCKED_SETTING_METADATA).toStringList();
    lockedCategories.removeOne(item->categoryName());
    if (value.toBool())
      lockedCategories.push_back(item->categoryName());
    settings_->setValue(settingsKey_, lockedCategories);
  }

  // Logic below needs to change if this assert triggers.  Basically, GUI may
  // update without the filter updating, but not vice versa.
  assert(rv || !wasEdited);
  if (rv)
  {
    // Update the GUI
    emit dataChanged(idx, idx);

    // Alert users who are listening
    if (wasEdited)
    {
      // Parent index, if it exists, is a category and might have updated its color data()
      const QModelIndex parentIndex = idx.parent();
      if (parentIndex.isValid())
        emit dataChanged(parentIndex, parentIndex);
      emitChildrenDataChanged_(idx);

      emit filterChanged(*filter_);
      emit filterEdited(*filter_);
    }
    else
    {
      // Should only happen in cases where EXCLUDE got changed, but no filter was edited
      assert(!idx.parent().isValid());
      emitChildrenDataChanged_(idx);
      emit excludeEdited(item->nameInt(), item->isUnlistedValueChecked());
    }
  }
  return rv;
}

void CategoryTreeModel::setFilter(const simData::CategoryFilter& filter)
{
  // Check the data store; if it's set in filter and different from ours, update
  if (filter.getDataStore() && filter.getDataStore() != dataStore_)
    setDataStore(filter.getDataStore());

  // Avoid no-op
  simData::CategoryFilter simplified(filter);
  simplified.simplify();
  if (filter_ != NULL && simplified == *filter_)
    return;

  // Do a two step assignment so that we don't automatically get auto-update
  if (filter_ == NULL)
    filter_ = new simData::CategoryFilter(filter.getDataStore());
  filter_->assign(simplified, false);

  const int categoriesSize = categories_.size();
  if (categoriesSize == 0)
  {
    // This means we have a simplified filter that is DIFFERENT from our current
    // filter, AND it means we have no items in the GUI.  It means we're out of
    // sync and something is not right.  Check into it.
    assert(0);
    return;
  }

  // Update to the filter, but detect which rows changed so we can simplify dataChanged()
  // for performance reasons.  This will prevent the display from updating too much.
  int firstChangeRow = -1;
  int lastChangeRow = -1;
  for (int k = 0; k < categoriesSize; ++k)
  {
    // Detect change and record the row number
    if (categories_[k]->updateTo(*filter_) != 0)
    {
      if (firstChangeRow == -1)
        firstChangeRow = k;
      lastChangeRow = k;
    }
  }
  // This shouldn't happen because we checked the simplified filters.  If this
  // assert triggers, then we have a change in filter (detected above) but the
  // GUI didn't actually change.  Maybe filter compare failed, or updateTo()
  // is returning incorrect values.
  assert(firstChangeRow != -1 && lastChangeRow != -1);
  if (firstChangeRow != -1 && lastChangeRow != -1)
  {
    emit dataChanged(index(firstChangeRow, 0), index(lastChangeRow, 0));
  }
  emit filterChanged(*filter_);
}

const simData::CategoryFilter& CategoryTreeModel::categoryFilter() const
{
  // Precondition of this method is that data store was set; filter must be non-NULL
  assert(filter_);
  return *filter_;
}

void CategoryTreeModel::setDataStore(simData::DataStore* dataStore)
{
  if (dataStore_ == dataStore)
    return;

  // Update the listeners on name manager as we change it
  if (dataStore_ != NULL)
    dataStore_->categoryNameManager().removeListener(listener_);
  dataStore_ = dataStore;
  if (dataStore_ != NULL)
    dataStore_->categoryNameManager().addListener(listener_);

  beginResetModel();

  // Clear out the internal storage on the tree
  categories_.deleteAll();
  categoryIntToItem_.clear();

  // Clear out the internal filter object
  const bool hadFilter = (filter_ != NULL && !filter_->isEmpty());
  delete filter_;
  filter_ = NULL;
  if (dataStore_)
  {
    filter_ = new simData::CategoryFilter(dataStore_);
    const simData::CategoryNameManager& nameManager = dataStore_->categoryNameManager();

    // Populate the GUI
    std::vector<int> nameInts;
    nameManager.allCategoryNameInts(nameInts);

    QString settingsKey;
    QStringList lockedCategories;
    if (settings_)
      lockedCategories = settings_->value(settingsKey_, LOCKED_SETTING_METADATA).toStringList();

    for (auto i = nameInts.begin(); i != nameInts.end(); ++i)
    {
      // Save the Category item and map it into our quick-search map
      CategoryItem* category = new CategoryItem(nameManager, *i);
      category->setFont(categoryFont_);
      categories_.push_back(category);
      categoryIntToItem_[*i] = category;

      // Create an item for "NO VALUE" since it won't be in the list of values we receive
      ValueItem* noValueItem = new ValueItem(nameManager, *i, simData::CategoryNameManager::NO_CATEGORY_VALUE_AT_TIME);
      category->addChild(noValueItem);

      // Save all the category values
      std::vector<int> valueInts;
      nameManager.allValueIntsInCategory(*i, valueInts);
      for (auto vi = valueInts.begin(); vi != valueInts.end(); ++vi)
      {
        ValueItem* valueItem = new ValueItem(nameManager, *i, *vi);
        category->addChild(valueItem);
      }

      // check settings to determine if newly added categories should be locked
      if (settings_)
        updateLockedState_(lockedCategories, *category);
    }
  }

  // Model reset is done
  endResetModel();

  // Alert listeners if we have a new filter
  if (hadFilter && filter_)
    emit filterChanged(*filter_);
}

void CategoryTreeModel::setSettings(Settings* settings, const QString& settingsKeyPrefix)
{
  settings_ = settings;
  settingsKey_ = settingsKeyPrefix + "/" + LOCKED_SETTING;

  if (!settings_)
    return;

  // check settings to determine if newly added categories should be locked
  QStringList lockedCategories = settings_->value(settingsKey_, LOCKED_SETTING_METADATA).toStringList();
  for (int i = 0; i < categories_.size(); ++i)
  {
    updateLockedState_(lockedCategories, *categories_[i]);
  }
}

void CategoryTreeModel::clearTree_()
{
  beginResetModel();
  categories_.deleteAll();
  categoryIntToItem_.clear();
  // need to manually clear the filter_ since auto update was turned off
  filter_->clear();
  endResetModel();
}

void CategoryTreeModel::addName_(int nameInt)
{
  assert(dataStore_ != NULL);

  // Create the tree item for the category
  const auto& nameManager = dataStore_->categoryNameManager();
  CategoryItem* category = new CategoryItem(nameManager, nameInt);
  category->setFont(categoryFont_);
  // check settings to determine if newly added categories should be locked
  if (settings_)
  {
    QStringList lockedCategories = settings_->value(settingsKey_, LOCKED_SETTING_METADATA).toStringList();
    updateLockedState_(lockedCategories, *category);
  }
  // Debug mode: Validate that there are no values in that category yet.  If this section
  // of code fails, then we'll need to add ValueItem entries for the category on creation.
#ifndef NDEBUG
  std::vector<int> valuesInCategory;
  dataStore_->categoryNameManager().allValueIntsInCategory(nameInt, valuesInCategory);
  // Assertion failure means we need to update this code to add the values.
  assert(valuesInCategory.empty());
#endif

  // About to update the GUI by adding a new item at the end
  beginInsertRows(QModelIndex(), categories_.size(), categories_.size());
  categories_.push_back(category);
  categoryIntToItem_[nameInt] = category;

  // Create an item for "NO VALUE" since it won't be in the list of values we receive
  ValueItem* noValueItem = new ValueItem(nameManager, nameInt, simData::CategoryNameManager::NO_CATEGORY_VALUE_AT_TIME);
  category->addChild(noValueItem);

  endInsertRows();
}

CategoryTreeModel::CategoryItem* CategoryTreeModel::findNameTree_(int nameInt) const
{
  auto i = categoryIntToItem_.find(nameInt);
  return (i == categoryIntToItem_.end()) ? NULL : i->second;
}

void CategoryTreeModel::updateLockedState_(const QStringList& lockedCategories, CategoryItem& category)
{
  if (!lockedCategories.contains(category.categoryName()))
    return;
  bool wasChanged = false;
  category.setData(true, CategoryTreeModel::ROLE_LOCKED_STATE, *filter_, wasChanged);
}

void CategoryTreeModel::addValue_(int nameInt, int valueInt)
{
  // Find the parent item
  TreeItem* nameItem = findNameTree_(nameInt);
  // Means we got a category that we don't know about; shouldn't happen.
  assert(nameItem);
  if (nameItem == NULL)
    return;

  // Create the value item
  ValueItem* valueItem = new ValueItem(dataStore_->categoryNameManager(), nameInt, valueInt);
  // Value item is unchecked, unless the parent has a regular expression
  if (nameItem->isRegExpApplied())
  {
    auto* reObject = filter_->getRegExp(nameInt);
    if (reObject)
      valueItem->setChecked(reObject->match(valueItem->valueString().toStdString()));
  }

  // Get the index for the name (parent), and add this new valueItem into the tree
  const QModelIndex nameIndex = createIndex(categories_.indexOf(static_cast<CategoryItem*>(nameItem)), 0, nameItem);
  beginInsertRows(nameIndex, nameItem->childCount(), nameItem->childCount());
  nameItem->addChild(valueItem);
  endInsertRows();
}

void CategoryTreeModel::processCategoryCounts(const simQt::CategoryCountResults& results)
{
  const int numCategories = categories_.size();
  int firstRowChanged = -1;
  int lastRowChanged = -1;
  const auto& allCats = results.allCategories;
  for (int k = 0; k < numCategories; ++k)
  {
    CategoryItem* categoryItem = categories_[k];
    const int nameInt = categoryItem->nameInt();

    // Might have a category added between when we fired off the call and when it finished
    const auto entry = allCats.find(nameInt);
    bool haveChange = false;

    // Updates the text for the category and its child values
    if (entry == allCats.end())
      haveChange = categoryItem->updateCounts(std::map<int, size_t>());
    else
      haveChange = categoryItem->updateCounts(entry->second);

    // Record the row for data changed
    if (haveChange)
    {
      if (firstRowChanged == -1)
        firstRowChanged = k;
      lastRowChanged = k;
    }
  }

  // Emit data changed
  if (firstRowChanged != -1)
    emit dataChanged(index(firstRowChanged, 0), index(lastRowChanged, 0));
}

void CategoryTreeModel::emitChildrenDataChanged_(const QModelIndex& parent)
{
  // Change all children
  const int numRows = rowCount(parent);
  const int numCols = columnCount(parent);
  if (numRows == 0 || numCols == 0)
    return;
  emit dataChanged(index(0, 0, parent), index(numRows - 1, numCols - 1, parent));
}

/////////////////////////////////////////////////////////////////////////

/** Style options for drawing a toggle switch */
struct StyleOptionToggleSwitch
{
  /** Rectangle to draw the switch in */
  QRect rect;
  /** Vertical space between drawn track and the rect */
  int trackMargin;
  /** Font to draw text in */
  QFont font;

  /** State: on (to the right) or off (to the left) */
  bool value;
  /** Locked state gives the toggle a disabled look */
  bool locked;

  /** Describes On|Off|Lock styles */
  struct StateStyle {
    /** Brush for painting the track */
    QBrush track;
    /** Brush for painting the thumb */
    QBrush thumb;
    /** Text to draw in the track */
    QString text;
    /** Color of text to draw */
    QColor textColor;
  };

  /** Style to use for ON state */
  StateStyle on;
  /** Style to use for OFF state */
  StateStyle off;
  /** Style to use for LOCK state */
  StateStyle lock;

  /** Initialize to default options */
  StyleOptionToggleSwitch()
    : trackMargin(0),
    value(false),
    locked(false)
  {
    // Teal colored track and thumb
    on.track = QColor(0, 150, 136);
    on.thumb = on.track;
    on.text = QObject::tr("Exclude");
    on.textColor = Qt::black;

    // Black and grey track and thumb
    off.track = Qt::black;
    off.thumb = QColor(200, 200, 200);
    off.text = QObject::tr("Match");
    off.textColor = Qt::white;

    // Disabled-looking grey track and thumb
    lock.track = QColor(100, 100, 100);
    lock.thumb = lock.track.color().lighter();
    lock.text = QObject::tr("Locked");
    lock.textColor = Qt::black;
  }
};

/////////////////////////////////////////////////////////////////////////

/** Responsible for internal layout and painting of a Toggle Switch widget */
class ToggleSwitchPainter
{
public:
  /** Paint the widget using the given options on the painter provided. */
  virtual void paint(const StyleOptionToggleSwitch& option, QPainter* painter) const;
  /** Returns a size hint for the toggle switch.  Uses option's rectangle height. */
  virtual QSize sizeHint(const StyleOptionToggleSwitch& option) const;

private:
  /** Stores rectangle zones for sub-elements of switch. */
  struct ChildRects
  {
    QRect track;
    QRect thumb;
    QRect text;
  };

  /** Calculates the rectangles for painting for each sub-element of the toggle switch. */
  void calculateRects_(const StyleOptionToggleSwitch& option, ChildRects& rects) const;
};

void ToggleSwitchPainter::paint(const StyleOptionToggleSwitch& option, QPainter* painter) const
{
  painter->save();

  // Adapted from https://stackoverflow.com/questions/14780517

  // Figure out positions of all subelements
  ChildRects r;
  calculateRects_(option, r);

  // Priority goes to the locked state style over on/off
  const StyleOptionToggleSwitch::StateStyle& valueStyle = (option.locked ? option.lock : (option.value ? option.on : option.off));

  // Draw the track
  painter->setPen(Qt::NoPen);
  painter->setBrush(valueStyle.track);
  painter->setOpacity(0.45);
  painter->setRenderHint(QPainter::Antialiasing, true);
  // Newer Qt with newer MSVC renders the rounded rect poorly if the rounding
  // pixels argument is half of pixel height or greater; reduce to 0.49
  const double halfHeight = r.track.height() * 0.49;
  painter->drawRoundedRect(r.track, halfHeight, halfHeight);

  // Draw the text next
  painter->setOpacity(1.0);
  painter->setPen(valueStyle.textColor);
  painter->setFont(option.font);
  painter->drawText(r.text, Qt::AlignHCenter | Qt::AlignVCenter, valueStyle.text);

  // Draw thumb on top of all
  painter->setPen(Qt::NoPen);
  painter->setBrush(valueStyle.thumb);
  painter->drawEllipse(r.thumb);

  painter->restore();
}

QSize ToggleSwitchPainter::sizeHint(const StyleOptionToggleSwitch& option) const
{
  // Count in the font text for width
  int textWidth = 0;
  QFontMetrics fontMetrics(option.font);
  if (!option.on.text.isEmpty() || !option.off.text.isEmpty())
  {
#if QT_VERSION < QT_VERSION_CHECK(5, 11, 0)
    const int onWidth = fontMetrics.width(option.on.text);
    const int offWidth = fontMetrics.width(option.off.text);
    const int lockWidth = fontMetrics.width(option.lock.text);
#else
    const int onWidth = fontMetrics.horizontalAdvance(option.on.text);
    const int offWidth = fontMetrics.horizontalAdvance(option.off.text);
    const int lockWidth = fontMetrics.horizontalAdvance(option.lock.text);
#endif
    textWidth = qMax(onWidth, offWidth);
    textWidth = qMax(lockWidth, textWidth);
  }

  // Best width depends on height
  int height = option.rect.height();
  if (height == 0)
    height = fontMetrics.height();

  const int desiredWidth = static_cast<int>(1.5 * option.rect.height()) + textWidth;
  return QSize(desiredWidth, height);
}

void ToggleSwitchPainter::calculateRects_(const StyleOptionToggleSwitch& option, ChildRects& rects) const
{
  // Track is centered about the rectangle
  rects.track = QRect(option.rect.adjusted(0, option.trackMargin, 0, -option.trackMargin));

  // Thumb should be 1 pixel shorter than the track on top and bottom
  rects.thumb = QRect(option.rect.adjusted(0, 1, 0, -1));
  rects.thumb.setWidth(rects.thumb.height());
  // Move thumb to the right if on and if category isn't locked
  if (option.value && !option.locked)
    rects.thumb.translate(rects.track.width() - rects.thumb.height(), 0);

  // Text is inside the rect, excluding the thumb area
  rects.text = QRect(option.rect);
  if (option.value)
    rects.text.setRight(rects.thumb.left());
  else
    rects.text.setLeft(rects.thumb.right());
  // Shift the text closer to center (thumb) to avoid being too close to edge
  rects.text.translate(option.value ? 1 : -1, 0);
}

/////////////////////////////////////////////////////////////////////////

/** Expected tree indentation.  Tree takes away parts of delegate for tree painting and we want to undo that. */
static const int TREE_INDENTATION = 20;

struct CategoryTreeItemDelegate::ChildRects
{
  QRect background;
  QRect checkbox;
  QRect branch;
  QRect text;
  QRect excludeToggle;
  QRect regExpButton;
};

CategoryTreeItemDelegate::CategoryTreeItemDelegate(QObject* parent)
  : QStyledItemDelegate(parent)
{
}

CategoryTreeItemDelegate::~CategoryTreeItemDelegate()
{
}

void CategoryTreeItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& inOption, const QModelIndex& index) const
{
  // Initialize a new option struct that has data from the QModelIndex
  QStyleOptionViewItem opt(inOption);
  initStyleOption(&opt, index);

  // Save the painter then draw based on type of node
  painter->save();
  if (!index.parent().isValid())
    paintCategory_(painter, opt, index);
  else
    paintValue_(painter, opt, index);
  painter->restore();
}

void CategoryTreeItemDelegate::paintCategory_(QPainter* painter, QStyleOptionViewItem& opt, const QModelIndex& index) const
{
  const QStyle* style = (opt.widget ? opt.widget->style() : qApp->style());

  // Calculate the rectangles for drawing
  ChildRects r;
  calculateRects_(opt, index, r);

  { // Draw a background for the whole row
    painter->setBrush(opt.backgroundBrush);
    painter->setPen(Qt::NoPen);
    painter->drawRect(r.background);
  }

  { // Draw the expand/collapse icon on left side
    QStyleOptionViewItem branchOpt(opt);
    branchOpt.rect = r.branch;
    branchOpt.state &= ~QStyle::State_MouseOver;
    style->drawPrimitive(QStyle::PE_IndicatorBranch, &branchOpt, painter);
  }

  { // Draw the text for the category
    opt.rect = r.text;
    style->drawControl(QStyle::CE_ItemViewItem, &opt, painter);
  }

  if (r.excludeToggle.isValid())
  { // Draw the toggle switch for changing EXCLUDE and INCLUDE
    StyleOptionToggleSwitch switchOpt;
    ToggleSwitchPainter switchPainter;
    switchOpt.rect = r.excludeToggle;
    switchOpt.locked = index.data(CategoryTreeModel::ROLE_LOCKED_STATE).toBool();
    switchOpt.value = (switchOpt.locked ? false : index.data(CategoryTreeModel::ROLE_EXCLUDE).toBool());
    switchPainter.paint(switchOpt, painter);
  }

  if (r.regExpButton.isValid())
  { // Draw the RegExp text box
    QStyleOptionButton buttonOpt;
    buttonOpt.rect = r.regExpButton;
    buttonOpt.text = tr("RegExp...");
    buttonOpt.state = QStyle::State_Enabled;
    if (clickedElement_ == SE_REGEXP_BUTTON && clickedIndex_ == index)
      buttonOpt.state |= QStyle::State_Sunken;
    else
      buttonOpt.state |= QStyle::State_Raised;
    style->drawControl(QStyle::CE_PushButton, &buttonOpt, painter);
  }
}

void CategoryTreeItemDelegate::paintValue_(QPainter* painter, QStyleOptionViewItem& opt, const QModelIndex& index) const
{
  const QStyle* style = (opt.widget ? opt.widget->style() : qApp->style());
  const bool isChecked = (index.data(Qt::CheckStateRole).toInt() == Qt::Checked);

  // Calculate the rectangles for drawing
  ChildRects r;
  calculateRects_(opt, index, r);
  opt.rect = r.text;

  // Draw a checked checkbox on left side of item if the item is checked
  if (isChecked)
  {
    // Move it to left side of widget
    QStyleOption checkOpt(opt);
    checkOpt.rect = r.checkbox;
    // Check the button, then draw
    checkOpt.state |= QStyle::State_On;
    style->drawPrimitive(QStyle::PE_IndicatorCheckBox, &checkOpt, painter);

    // Checked category values also show up bold
    opt.font.setBold(true);
  }

  // Category values that are hovered are shown as underlined in link color (blue usually)
  if (opt.state.testFlag(QStyle::State_MouseOver) && opt.state.testFlag(QStyle::State_Enabled))
  {
    opt.font.setUnderline(true);
    opt.palette.setBrush(QPalette::Text, opt.palette.color(QPalette::Link));
  }

  // Turn off the check indicator unconditionally, then draw the item
  opt.features &= ~QStyleOptionViewItem::HasCheckIndicator;
  style->drawControl(QStyle::CE_ItemViewItem, &opt, painter);
}

bool CategoryTreeItemDelegate::editorEvent(QEvent* evt, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index)
{
  if (index.isValid() && !index.parent().isValid())
    return categoryEvent_(evt, model, option, index);
  return valueEvent_(evt, model, option, index);
}

bool CategoryTreeItemDelegate::categoryEvent_(QEvent* evt, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index)
{
  // Cast may not be valid, depends on evt->type()
  const QMouseEvent* me = static_cast<const QMouseEvent*>(evt);

  switch (evt->type())
  {
  case QEvent::MouseButtonPress:
    // Only care about left presses.  All other presses are ignored.
    if (me->button() != Qt::LeftButton)
    {
      clickedIndex_ = QModelIndex();
      return false;
    }
    // Ignore event if category is locked
    if (index.data(CategoryTreeModel::ROLE_LOCKED_STATE).toBool())
    {
      clickedIndex_ = QModelIndex();
      return true;
    }

    clickedElement_ = hit_(me->pos(), option, index);
    // Eat the branch press and don't do anything on release
    if (clickedElement_ == SE_BRANCH)
    {
      clickedIndex_ = QModelIndex();
      emit expandClicked(index);
      return true;
    }
    clickedIndex_ = index;
    if (clickedElement_ == SE_REGEXP_BUTTON)
      return true;
    break;

  case QEvent::MouseButtonRelease:
  {
    // Ignore event if category is locked
    if (index.data(CategoryTreeModel::ROLE_LOCKED_STATE).toBool())
    {
      clickedIndex_ = QModelIndex();
      return true;
    }
    // Clicking on toggle should save the index to detect release on the toggle
    const auto newHit = hit_(me->pos(), option, index);
    // Must match button, index, and element clicked
    if (me->button() == Qt::LeftButton && clickedIndex_ == index && newHit == clickedElement_)
    {
      // Toggle button should, well, toggle
      if (clickedElement_ == SE_EXCLUDE_TOGGLE)
      {
        QVariant oldState = index.data(CategoryTreeModel::ROLE_EXCLUDE);
        if (index.flags().testFlag(Qt::ItemIsEnabled))
          model->setData(index, !oldState.toBool(), CategoryTreeModel::ROLE_EXCLUDE);
        clickedIndex_ = QModelIndex();
        return true;
      }
      else if (clickedElement_ == SE_REGEXP_BUTTON)
      {
        // Need to talk to the tree itself to do the input GUI, so pass this off as a signal
        emit editRegExpClicked(index);
        clickedIndex_ = QModelIndex();
        return true;
      }
    }
    clickedIndex_ = QModelIndex();
    break;
  }

  case QEvent::MouseButtonDblClick:
    // Ignore event if category is locked
    if (index.data(CategoryTreeModel::ROLE_LOCKED_STATE).toBool())
    {
      clickedIndex_ = QModelIndex();
      return true;
    }

    clickedIndex_ = QModelIndex();
    clickedElement_ = hit_(me->pos(), option, index);
    // Ignore double click on the toggle, branch, and RegExp buttons, so that it doesn't cause expand/contract
    if (clickedElement_ == SE_EXCLUDE_TOGGLE || clickedElement_ == SE_BRANCH || clickedElement_ == SE_REGEXP_BUTTON)
      return true;
    break;

  default: // Many potential events not handled
    break;
  }

  return false;
}

bool CategoryTreeItemDelegate::valueEvent_(QEvent* evt, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index)
{
  if (evt->type() != QEvent::MouseButtonPress && evt->type() != QEvent::MouseButtonRelease)
    return false;
  // At this stage it's either a press or a release
  const QMouseEvent* me = static_cast<const QMouseEvent*>(evt);
  const bool isPress = (evt->type() == QEvent::MouseButtonPress);
  const bool isRelease = !isPress;

  // Determine whether we care about the event
  bool usefulEvent = true;
  if (me->button() != Qt::LeftButton)
    usefulEvent = false;
  else if (isRelease && clickedIndex_ != index)
    usefulEvent = false;
  // Should have a check state; if not, that's weird, return out
  QVariant checkState = index.data(Qt::CheckStateRole);
  if (!checkState.isValid())
    usefulEvent = false;

  // Clear out the model index before returning
  if (!usefulEvent)
  {
    clickedIndex_ = QModelIndex();
    return false;
  }

  // If it's a press, save the index for later.  Note we don't use clickedElement_
  if (isPress)
    clickedIndex_ = index;
  else
  {
    // Invert the state and send it as an updated check
    Qt::CheckState newState = (checkState.toInt() == Qt::Checked) ? Qt::Unchecked : Qt::Checked;
    if (index.flags().testFlag(Qt::ItemIsEnabled))
      model->setData(index, newState, Qt::CheckStateRole);
    clickedIndex_ = QModelIndex();
  }
  return true;
}

void CategoryTreeItemDelegate::calculateRects_(const QStyleOptionViewItem& option, const QModelIndex& index, ChildRects& rects) const
{
  rects.background = option.rect;

  const bool isValue = index.isValid() && index.parent().isValid();
  if (isValue)
  {
    rects.background.setLeft(0);
    rects.checkbox = rects.background;
    rects.checkbox.setRight(TREE_INDENTATION);
    rects.excludeToggle = QRect();
    rects.regExpButton = QRect();

    // Text takes up everything to the right of the checkbox
    rects.text = rects.background.adjusted(TREE_INDENTATION, 0, 0, 0);
  }
  else
  {
    // Branch is the > or v indicator for expanding
    rects.branch = rects.background;
    rects.branch.setRight(rects.branch.left() + rects.branch.height());

    // Calculate the width given the rectangle of height, for the toggle switch
    const bool haveRegExp = !index.data(CategoryTreeModel::ROLE_REGEXP_STRING).toString().isEmpty();
    if (haveRegExp)
    {
      rects.excludeToggle = QRect();
      rects.regExpButton = rects.background.adjusted(0, 1, -1, -1);
      rects.regExpButton.setLeft(rects.regExpButton.right() - 70);
    }
    else
    {
      rects.excludeToggle = rects.background.adjusted(0, 1, -1, -1);
      ToggleSwitchPainter switchPainter;
      StyleOptionToggleSwitch switchOpt;
      switchOpt.rect = rects.excludeToggle;
      const QSize toggleSize = switchPainter.sizeHint(switchOpt);
      // Set the left side appropriately
      rects.excludeToggle.setLeft(rects.excludeToggle.right() - toggleSize.width());
    }

    // Text takes up everything to the right of the branch button until the exclude toggle
    rects.text = rects.background;
    rects.text.setLeft(rects.branch.right());
    if (haveRegExp)
      rects.text.setRight(rects.regExpButton.left());
    else
      rects.text.setRight(rects.excludeToggle.left());
  }
}

CategoryTreeItemDelegate::SubElement CategoryTreeItemDelegate::hit_(const QPoint& pos, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
  // Calculate the various rectangles
  ChildRects r;
  calculateRects_(option, index, r);

  if (r.excludeToggle.isValid() && r.excludeToggle.contains(pos))
    return SE_EXCLUDE_TOGGLE;
  if (r.regExpButton.isValid() && r.regExpButton.contains(pos))
    return SE_REGEXP_BUTTON;
  if (r.checkbox.isValid() && r.checkbox.contains(pos))
    return SE_CHECKBOX;
  if (r.branch.isValid() && r.branch.contains(pos))
    return SE_BRANCH;
  if (r.text.isValid() && r.text.contains(pos))
    return SE_TEXT;
  // Background encompasses all, so if we're not here we're in NONE
  if (r.background.isValid() && r.background.contains(pos))
    return SE_BACKGROUND;
  return SE_NONE;
}

bool CategoryTreeItemDelegate::helpEvent(QHelpEvent* evt, QAbstractItemView* view, const QStyleOptionViewItem& option, const QModelIndex& index)
{
  if (evt->type() == QEvent::ToolTip)
  {
    // Special tooltip for the EXCLUDE filter
    const SubElement subElement = hit_(evt->pos(), option, index);
    if (subElement == SE_EXCLUDE_TOGGLE)
    {
      QToolTip::showText(evt->globalPos(), simQt::formatTooltip(tr("Exclude"),
        tr("When on, Exclude mode will omit all entities that match your selected values.<p>When off, the filter will match all entities that have one of your checked category values.<p>Exclude mode does not show entity counts.")),
        view);
      return true;
    }
    else if (subElement == SE_REGEXP_BUTTON)
    {
      QToolTip::showText(evt->globalPos(), simQt::formatTooltip(tr("Set Regular Expression"),
        tr("A regular expression has been set for this category.  Use this button to change the category's regular expression.")),
        view);
      return true;
    }
  }
  return QStyledItemDelegate::helpEvent(evt, view, option, index);
}

/////////////////////////////////////////////////////////////////////////
/**
* Class that listens for entity events in the DataStore, and
* informs the parent when they happen.
*/
class CategoryFilterWidget::DataStoreListener : public simData::DataStore::Listener
{
public:
  explicit DataStoreListener(CategoryFilterWidget& parent)
    : parent_(parent)
  {};

  virtual void onAddEntity(simData::DataStore *source, simData::ObjectId newId, simData::ObjectType ot)
  {
    parent_.countDirty_ = true;
  }
  virtual void onRemoveEntity(simData::DataStore *source, simData::ObjectId newId, simData::ObjectType ot)
  {
    parent_.countDirty_ = true;
  }
  virtual void onCategoryDataChange(simData::DataStore *source, simData::ObjectId changedId, simData::ObjectType ot)
  {
    parent_.countDirty_ = true;
  }

  // Fulfill the interface
  virtual void onNameChange(simData::DataStore *source, simData::ObjectId changeId) {}
  virtual void onScenarioDelete(simData::DataStore* source) {}
  virtual void onPrefsChange(simData::DataStore *source, simData::ObjectId id) {}
  virtual void onTimeChange(simData::DataStore *source) {}
  virtual void onFlush(simData::DataStore* source, simData::ObjectId id) {}

private:
  CategoryFilterWidget& parent_;
};

/////////////////////////////////////////////////////////////////////////

CategoryFilterWidget::CategoryFilterWidget(QWidget* parent)
  : QWidget(parent),
    activeFiltering_(false),
    showEntityCount_(false),
    counter_(NULL),
    setRegExpAction_(NULL),
    countDirty_(true)
{
  setWindowTitle("Category Data Filter");
  setObjectName("CategoryFilterWidget");

  treeModel_ = new simQt::CategoryTreeModel(this);
  proxy_ = new simQt::CategoryProxyModel(this);
  proxy_->setSourceModel(treeModel_);
  proxy_->setSortRole(simQt::CategoryTreeModel::ROLE_SORT_STRING);
  proxy_->sort(0);

  treeView_ = new QTreeView(this);
  treeView_->setObjectName("CategoryFilterTree");
  treeView_->setFocusPolicy(Qt::NoFocus);
  treeView_->setEditTriggers(QAbstractItemView::NoEditTriggers);
  treeView_->setIndentation(0);
  treeView_->setAllColumnsShowFocus(true);
  treeView_->setHeaderHidden(true);
  treeView_->setModel(proxy_);
  treeView_->setMouseTracking(true);

  simQt::CategoryTreeItemDelegate* itemDelegate = new simQt::CategoryTreeItemDelegate(this);
  treeView_->setItemDelegate(itemDelegate);

  setRegExpAction_ = new QAction(tr("Set Regular Expression..."), this);
  connect(setRegExpAction_, SIGNAL(triggered()), this, SLOT(setRegularExpression_()));
  clearRegExpAction_ = new QAction(tr("Clear Regular Expression"), this);
  connect(clearRegExpAction_, SIGNAL(triggered()), this, SLOT(clearRegularExpression_()));

  QAction* separator1 = new QAction(this);
  separator1->setSeparator(true);

  QAction* resetAction = new QAction(tr("Reset"), this);
  connect(resetAction, SIGNAL(triggered()), this, SLOT(resetFilter_()));
  QAction* separator2 = new QAction(this);
  separator2->setSeparator(true);

  toggleLockCategoryAction_ = new QAction(tr("Lock Category"), this);
  connect(toggleLockCategoryAction_, SIGNAL(triggered()), this, SLOT(toggleLockCategory_()));

  QAction* separator3 = new QAction(this);
  separator3->setSeparator(true);

  QAction* collapseAction = new QAction(tr("Collapse Values"), this);
  connect(collapseAction, SIGNAL(triggered()), treeView_, SLOT(collapseAll()));
  collapseAction->setIcon(QIcon(":/simQt/images/Collapse.png"));

  QAction* expandAction = new QAction(tr("Expand Values"), this);
  connect(expandAction, SIGNAL(triggered()), this, SLOT(expandUnlockedCategories_()));
  expandAction->setIcon(QIcon(":/simQt/images/Expand.png"));

  treeView_->setContextMenuPolicy(Qt::CustomContextMenu);
  treeView_->addAction(setRegExpAction_);
  treeView_->addAction(clearRegExpAction_);
  treeView_->addAction(separator1);
  treeView_->addAction(resetAction);
  treeView_->addAction(separator2);
  treeView_->addAction(toggleLockCategoryAction_);
  treeView_->addAction(separator3);
  treeView_->addAction(collapseAction);
  treeView_->addAction(expandAction);

  simQt::SearchLineEdit* search = new simQt::SearchLineEdit(this);
  search->setPlaceholderText(tr("Search Category Data"));

  QVBoxLayout *layout = new QVBoxLayout(this);
  layout->setObjectName("CategoryFilterWidgetVBox");
  layout->setMargin(0);
  layout->addWidget(search);
  layout->addWidget(treeView_);

  connect(treeView_, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showContextMenu_(QPoint)));
  connect(treeModel_, SIGNAL(filterChanged(simData::CategoryFilter)), this, SIGNAL(filterChanged(simData::CategoryFilter)));
  connect(treeModel_, SIGNAL(filterEdited(simData::CategoryFilter)), this, SIGNAL(filterEdited(simData::CategoryFilter)));
  connect(proxy_, SIGNAL(rowsInserted(QModelIndex, int, int)), this, SLOT(expandDueToProxy_(QModelIndex, int, int)));
  connect(search, SIGNAL(textChanged(QString)), this, SLOT(expandAfterFilterEdited_(QString)));
  connect(search, SIGNAL(textChanged(QString)), proxy_, SLOT(setFilterText(QString)));
  connect(itemDelegate, SIGNAL(expandClicked(QModelIndex)), this, SLOT(toggleExpanded_(QModelIndex)));
  connect(itemDelegate, SIGNAL(editRegExpClicked(QModelIndex)), this, SLOT(showRegExpEditGui_(QModelIndex)));

  // timer is connected by setShowEntityCount below; it must be constructed before setShowEntityCount
  auto recountTimer = new QTimer(this);
  recountTimer->setSingleShot(false);
  recountTimer->setInterval(3000);
  connect(recountTimer, SIGNAL(timeout()), this, SLOT(recountCategories_()));
  recountTimer->start();

  // Entity filtering is on by default
  setShowEntityCount(true);

  dsListener_.reset(new CategoryFilterWidget::DataStoreListener(*this));
}

CategoryFilterWidget::~CategoryFilterWidget()
{
  if (categoryFilter().getDataStore())
    categoryFilter().getDataStore()->removeListener(dsListener_);
}

void CategoryFilterWidget::setDataStore(simData::DataStore* dataStore)
{
  simData::DataStore* prevDataStore = categoryFilter().getDataStore();
  if (prevDataStore == dataStore)
    return;

  if (prevDataStore)
    prevDataStore->removeListener(dsListener_);

  treeModel_->setDataStore(dataStore);
  counter_->setFilter(categoryFilter());

  if (dataStore)
    dataStore->addListener(dsListener_);
}

void CategoryFilterWidget::setSettings(Settings* settings, const QString& settingsKeyPrefix)
{
  treeModel_->setSettings(settings, settingsKeyPrefix);
}

const simData::CategoryFilter& CategoryFilterWidget::categoryFilter() const
{
  return treeModel_->categoryFilter();
}

void CategoryFilterWidget::setFilter(const simData::CategoryFilter& categoryFilter)
{
  treeModel_->setFilter(categoryFilter);
}

void CategoryFilterWidget::processCategoryCounts(const simQt::CategoryCountResults& results)
{
  treeModel_->processCategoryCounts(results);
}

bool CategoryFilterWidget::showEntityCount() const
{
  return showEntityCount_;
}

void CategoryFilterWidget::setShowEntityCount(bool fl)
{
  if (fl == showEntityCount_)
    return;
  showEntityCount_ = fl;

  // Clear out the old counter
  delete counter_;
  counter_ = NULL;

  // Create a new counter and configure it
  if (showEntityCount_)
  {
    counter_ = new simQt::AsyncCategoryCounter(this);
    connect(counter_, SIGNAL(resultsReady(simQt::CategoryCountResults)), this, SLOT(processCategoryCounts(simQt::CategoryCountResults)));
    connect(treeModel_, SIGNAL(filterChanged(simData::CategoryFilter)), counter_, SLOT(setFilter(simData::CategoryFilter)));
    connect(treeModel_, SIGNAL(rowsInserted(QModelIndex, int, int)), counter_, SLOT(asyncCountEntities()));
    counter_->setFilter(categoryFilter());
  }
  else
  {
    treeModel_->processCategoryCounts(simQt::CategoryCountResults());
  }
}

void CategoryFilterWidget::expandAfterFilterEdited_(const QString& filterText)
{
  if (filterText.isEmpty())
  {
    // Just removed the last character of a search so collapse all to hide everything
    if (activeFiltering_)
      treeView_->collapseAll();

    activeFiltering_ = false;
  }
  else
  {
    // Just started a search so expand all to make everything visible
    if (!activeFiltering_)
      treeView_->expandAll();

    activeFiltering_ = true;
  }
}

void CategoryFilterWidget::expandDueToProxy_(const QModelIndex& parentIndex, int to, int from)
{
  // Only expand when we're actively filtering, because we want
  // to see rows that match the active filter as they show up
  if (!activeFiltering_)
    return;

  bool isCategory = !parentIndex.isValid();
  if (isCategory)
  {
    // The category names are the "to" to "from" and they just showed up, so expand them
    for (int ii = to; ii <= from; ++ii)
    {
      QModelIndex catIndex = proxy_->index(ii, 0, parentIndex);
      treeView_->expand(catIndex);
    }
  }
  else
  {
    if (activeFiltering_)
    {
      // Adding a category value; make sure it is visible by expanding its parent
      if (!treeView_->isExpanded(parentIndex))
        treeView_->expand(parentIndex);
    }
  }
}

void CategoryFilterWidget::toggleExpanded_(const QModelIndex& proxyIndex)
{
  treeView_->setExpanded(proxyIndex, !treeView_->isExpanded(proxyIndex));
}

void CategoryFilterWidget::resetFilter_()
{
  // Create a new empty filter using same data store
  const simData::CategoryFilter newFilter(treeModel_->categoryFilter().getDataStore());
  treeModel_->setFilter(newFilter);

  // Tree would have sent out a changed signal, but not an edited signal (because we are
  // doing this programmatically).  That's OK, but we need to send out an edited signal.
  emit filterEdited(treeModel_->categoryFilter());
}

void CategoryFilterWidget::showContextMenu_(const QPoint& point)
{
  QMenu contextMenu(this);
  contextMenu.addActions(treeView_->actions());

  // Mark the RegExp and Lock actions enabled or disabled based on current state
  const QModelIndex idx = treeView_->indexAt(point);
  const bool emptyRegExp = idx.data(CategoryTreeModel::ROLE_REGEXP_STRING).toString().isEmpty();
  const bool locked = idx.data(CategoryTreeModel::ROLE_LOCKED_STATE).toBool();
  if (locked && !emptyRegExp)
    assert(0); // Should not be possible to have a RegExp set on a locked category
  setRegExpAction_->setProperty("index", idx);
  setRegExpAction_->setEnabled(idx.isValid() && !locked); // RegExp is disabled while locked
  // Mark the Clear RegExp action similarly
  clearRegExpAction_->setProperty("index", idx);
  clearRegExpAction_->setEnabled(idx.isValid() && !emptyRegExp && !locked); // RegExp is disabled while locked

  // Store the index in the Toggle Lock Category action
  toggleLockCategoryAction_->setProperty("index", idx);
  toggleLockCategoryAction_->setEnabled(idx.isValid() && emptyRegExp); // Locking is disabled while locked
  // Update the text based on the current lock state
  toggleLockCategoryAction_->setText(locked ? tr("Unlock Category") : tr("Lock Category"));

  // Show the menu
  contextMenu.exec(treeView_->mapToGlobal(point));

  // Clear the index property and disable
  setRegExpAction_->setProperty("index", QVariant());
  setRegExpAction_->setEnabled(false);
  clearRegExpAction_->setProperty("index", idx);
  clearRegExpAction_->setEnabled(false);
  toggleLockCategoryAction_->setProperty("index", QVariant());
}

void CategoryFilterWidget::setRegularExpression_()
{
  // Make sure we have a sender and can pull out the index.  If not, return
  QObject* senderObject = sender();
  if (senderObject == NULL)
    return;
  QModelIndex index = senderObject->property("index").toModelIndex();
  if (index.isValid())
    showRegExpEditGui_(index);
}

void CategoryFilterWidget::showRegExpEditGui_(const QModelIndex& index)
{
  // Grab category name and old regexp, then ask user for new value
  const QString oldRegExp = index.data(CategoryTreeModel::ROLE_REGEXP_STRING).toString();
  const QString categoryName = index.data(CategoryTreeModel::ROLE_CATEGORY_NAME).toString();

  // pop up dialog with a entity filter line edit that supports formatting regexp
  QDialog optionsDialog(this);
  optionsDialog.setWindowTitle(tr("Set Regular Expression"));
  optionsDialog.setWindowFlags(optionsDialog.windowFlags() & ~Qt::WindowContextHelpButtonHint);

  QLayout* layout = new QVBoxLayout(&optionsDialog);
  QLabel* label = new QLabel(tr("Set '%1' value regular expression:").arg(categoryName), &optionsDialog);
  layout->addWidget(label);
  EntityFilterLineEdit* lineEdit = new EntityFilterLineEdit(&optionsDialog);
  lineEdit->setRegexOnly(true);
  lineEdit->setText(oldRegExp);
  lineEdit->setToolTip(
    tr("Regular expressions can be applied to categories in a filter.  Categories with regular expression filters will match only the values that match the regular expression."
    "<p>This popup changes the regular expression value for the category '%1'."
    "<p>An empty string can be used to clear the regular expression and return to normal matching mode.").arg(categoryName));
  layout->addWidget(lineEdit);
  QDialogButtonBox buttons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &optionsDialog);
  connect(lineEdit, SIGNAL(isValidChanged(bool)), buttons.button(QDialogButtonBox::Ok), SLOT(setEnabled(bool)));
  connect(&buttons, SIGNAL(accepted()), &optionsDialog, SLOT(accept()));
  connect(&buttons, SIGNAL(rejected()), &optionsDialog, SLOT(reject()));
  layout->addWidget(&buttons);
  optionsDialog.setLayout(layout);
  if (optionsDialog.exec() == QDialog::Accepted && lineEdit->text() != oldRegExp)
  {
    // index.model() is const because changes to the model might invalidate indices.  Since we know this
    // and no longer use the index after this call, it is safe to use const_cast here to use setData().
    QAbstractItemModel* model = const_cast<QAbstractItemModel*>(index.model());
    model->setData(index, lineEdit->text(), CategoryTreeModel::ROLE_REGEXP_STRING);
  }
}

void CategoryFilterWidget::clearRegularExpression_()
{
  // Make sure we have a sender and can pull out the index.  If not, return
  QObject* senderObject = sender();
  if (senderObject == NULL)
    return;
  QModelIndex index = senderObject->property("index").toModelIndex();
  if (!index.isValid())
    return;
  // index.model() is const because changes to the model might invalidate indices.  Since we know this
  // and no longer use the index after this call, it is safe to use const_cast here to use setData().
  QAbstractItemModel* model = const_cast<QAbstractItemModel*>(index.model());
  model->setData(index, QString(""), CategoryTreeModel::ROLE_REGEXP_STRING);
}

void CategoryFilterWidget::toggleLockCategory_()
{
  // Make sure we have a sender and can pull out the index.  If not, return
  QObject* senderObject = sender();
  if (senderObject == NULL)
    return;
  QModelIndex index = senderObject->property("index").toModelIndex();
  if (!index.isValid())
    return;

  const bool locked = index.data(CategoryTreeModel::ROLE_LOCKED_STATE).toBool();

  if (!locked)
  {
    // If index is a value, get its category parent
    if (index.parent().isValid())
      index = index.parent();
    if (!index.isValid())
    {
      assert(0); // value index should have a valid parent
      return;
    }

    // Collapse the category
    treeView_->setExpanded(index, false);
  }

  // index.model() is const because changes to the model might invalidate indices.  Since we know this
  // and no longer use the index after this call, it is safe to use const_cast here to use setData().
  QAbstractItemModel* model = const_cast<QAbstractItemModel*>(index.model());
  // Unlock the category
  model->setData(index, !locked, CategoryTreeModel::ROLE_LOCKED_STATE);
}

void CategoryFilterWidget::expandUnlockedCategories_()
{
  // Expand each category if it isn't locked
  for (int i = 0; i < proxy_->rowCount(); ++i)
  {
    const QModelIndex& idx = proxy_->index(i, 0);
    if (!idx.data(CategoryTreeModel::ROLE_LOCKED_STATE).toBool())
      treeView_->setExpanded(idx, true);
  }
}

void CategoryFilterWidget::recountCategories_()
{
  if (countDirty_)
  {
    if (showEntityCount_ && counter_)
      counter_->asyncCountEntities();
    countDirty_ = false;
  }
}

}
