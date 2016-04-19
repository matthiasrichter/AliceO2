//-*- Mode: C++ -*-

#ifndef FACTORY_H
#define FACTORY_H
//****************************************************************************
//* This file is free software: you can redistribute it and/or modify        *
//* it under the terms of the GNU General Public License as published by     *
//* the Free Software Foundation, either version 3 of the License, or        *
//* (at your option) any later version.                                      *
//*                                                                          *
//* The authors make no claims about the suitability of this software for    *
//* any purpose. It is provided "as is" without express or implied warranty. *
//****************************************************************************

//  @file   Factory.h
//  @author Matthias Richter
//  @since  2016-04-15
//  @brief  Generic Factory

/* scratchpad
macro ALICEO2_AUTO_DEVICE to create most of the device
probably template implementation to allow different compilations: debugging/logging, transport,
add a creator function to the factory
 */

#include <map> // std::map
#include <functional> // std::function

namespace AliceO2 {

/**
 * @class Factory
 * @brief A generic factory for a class type identified by keys
 *
 * The class implements an internal singleton, the interface is accessible
 * through an object of Factory.
 * 
 * Usage:
 * <pre>
 *   AliceO2::Factory<SomeClass, const char*> factory;
 *   factory.Register("TestClass1", CreateTestClass1);
 *   // ...
 *   SomeClass* someObj = factory.Create("TestClass1");
 * <pre>
 */
template<class T, typename Key>
class Factory {
public:
  typedef std::function<T*()> CreatorT;

  /// register a product creator
  int Register(Key key, CreatorT func) {
    return Instance().Register(key, func);
  }

  /// create the product identified by key
  T* Create(Key key) const {
    return Instance().Create(key);
  }

protected:
  /// worker class for factory instance, internal use only
  class FactoryInstance {
  public:
    // std constructor
    FactoryInstance() {}

    /// register a product creator
    int Register(Key key, CreatorT func) {
      auto element = mCreators.find(key);
      if (element != mCreators.end()) {
        // this creator has been registered already
        if (element->second == fun) return 0;
        // different creator with same key
        // TODO: define error policy by user
        return -1;
      }
      mCreators.insert(std::pair<Key, CreatorT>(key, func));
      return 0;
    }

    /// create the product identified by key
    T* Create(Key key) const {
      auto element = mCreators.find(key);
      if (element == mCreators.end()) return nullptr;
      return (element->second)();
    }

  private:
    // no copy constructor
    FactoryInstance(const FactoryInstance&);
    // forbid anyone to delete the instance
    ~FactoryInstance();

    /// list of creators
    std::map<Key, CreatorT> mCreators;
  };

  /// return instance of the factory worker
  static FactoryInstance& Instance() {
    // this instance is never deleted, though it is not a mempry leak
    static FactoryInstance* instance = nullptr;

    // TODO: safe only in single thread model
    if (!instance)
      instance = new FactoryInstance;
    return *instance;
  }

private:

};
} // namespace AliceO2
#endif // FACTORY_H
