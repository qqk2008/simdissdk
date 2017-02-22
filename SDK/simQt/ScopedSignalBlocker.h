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
#ifndef SIMQT_SCOPEDSIGNALBLOCKER_H
#define SIMQT_SCOPEDSIGNALBLOCKER_H

#include <QObject>

namespace simQt
{

/**
 * Blocks signals to the specified object.  On destruction, restores the previously
 * saved blocked state.  This is useful to avoid never ending notification loops in
 * QObject instances, while ensuring that previously blocking state is reinstated at
 * the end of the current scope.  For example:
 *
 *  ScopedSignalBlocker blockEditSignals(*ui_->editText);
 *  ui_->editText->setText("Text not announced through signals");
 *
 */
class /* SDKQT_EXPORT */ ScopedSignalBlocker
{
public:
  /** Blocks signals coming from the specified object, as long as this instance is in scope. */
  ScopedSignalBlocker(QObject& obj, bool blockSignals=true)
    : object_(obj)
  {
    blocked_ = object_.blockSignals(blockSignals);
  }

  /** Restores the previous block state. */
  virtual ~ScopedSignalBlocker()
  {
    object_.blockSignals(blocked_);
  }

private:
  /** Object being blocked */
  QObject& object_;
  /** Previous block state */
  bool blocked_;
};

}

#endif /* SIMQT_SCOPEDSIGNALBLOCKER_H */
