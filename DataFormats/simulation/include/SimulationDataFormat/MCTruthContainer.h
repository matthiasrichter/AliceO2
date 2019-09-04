// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file MCTruthContainer.h
/// \brief Definition of a container to keep Monte Carlo truth external to simulation objects
/// \author Sandro Wenzel - June 2017

#ifndef ALICEO2_DATAFORMATS_MCTRUTH_H_
#define ALICEO2_DATAFORMATS_MCTRUTH_H_

#include <TNamed.h>
#include <cassert>
#include <stdexcept>
#include <algorithm>
#include <gsl/gsl> // for guideline support library; array_view
#include <type_traits>
#include <string.h> // memmove

namespace o2
{
namespace dataformats
{
// a simple struct having information about truth elements for particular indices:
// how many associations we have and where they start in the storage
struct MCTruthHeaderElement {
  MCTruthHeaderElement() = default; // for ROOT IO

  MCTruthHeaderElement(uint i) : index(i) {}
  uint index = -1; // the index into the actual MC track storage (-1 if invalid)
  ClassDefNV(MCTruthHeaderElement, 1);
};

// A container to hold and manage MC truth information/labels.
// The actual MCtruth type is a generic template type and can be supplied by the user
// It is meant to manage associations from one "dataobject" identified by an index into an array
// to multiple TruthElements

template <typename TruthElement>
class MCTruthContainer
{
 private:
  using self_type = MCTruthContainer<TruthElement>;
  using IndexSizeType = size_t;
  using IndexElementType = MCTruthHeaderElement;
  using TruthElementType = TruthElement;

  std::vector<char> mData;

  size_t getSize(uint dataindex) const
  {
    // calculate size / number of labels from a difference in pointed indices
    const auto size = (dataindex < getIndexedSize() - 1)
      ? getMCTruthHeader(dataindex + 1).index - getMCTruthHeader(dataindex).index
      : getNElements() - getMCTruthHeader(dataindex).index;
    return size;
  }

  size_t getTruthElementArrayOffset() const
  {
    size_t offset = getIndexedSize() * sizeof(IndexElementType) + sizeof(IndexSizeType);
    assert(mData.size() >= sizeof(IndexSizeType));
    assert(offset <= mData.size());
    return offset;
  }

  TruthElementType const& operator[](size_t index) const
  {
    auto const position = getTruthElementArrayOffset() + index * sizeof(TruthElementType);
    assert(mData.size() > position);
    return *reinterpret_cast<TruthElementType const*>(mData.data() + position);
  }

  TruthElementType& operator[](size_t index)
  {
    auto const position = getTruthElementArrayOffset() + index * sizeof(TruthElementType);
    assert(mData.size() > position);
    return *reinterpret_cast<TruthElementType*>(mData.data() + position);
  }

 public:
  // constructor
  MCTruthContainer() : mData(sizeof(IndexSizeType)) {};
  // destructor
  ~MCTruthContainer() = default;
  // copy constructor
  MCTruthContainer(const MCTruthContainer& other) = default;
  // move constructor
  MCTruthContainer(MCTruthContainer&& other) = default;
  // assignment operator
  MCTruthContainer& operator=(const MCTruthContainer& other) = default;
  // move assignment operator
  MCTruthContainer& operator=(MCTruthContainer&& other) = default;

  // access
  MCTruthHeaderElement const& getMCTruthHeader(uint dataindex) const
  {
    if (dataindex >= getIndexedSize()) {
      throw std::out_of_range("index " + std::to_string(dataindex) + " out of range " + std::to_string(getIndexedSize()));
    }
    return *reinterpret_cast<MCTruthHeaderElement const*>(mData.data() + sizeof(IndexSizeType) + dataindex * sizeof(IndexElementType));
  }
  // access the element directly (can be encapsulated better away)... needs proper element index
  // which can be obtained from the MCTruthHeader startposition and size
  TruthElement const& getElement(uint elementindex) const { return (*this)[elementindex]; }
  // return the number of original data indexed here
  IndexSizeType getIndexedSize() const
  {
    if (mData.size() == 0) {
      // this is a fresh and empty index, create the size field
      const_cast<self_type*>(this)->mData.resize(sizeof(IndexSizeType), 0);
    }
    assert(mData.size() >= sizeof(IndexSizeType));
    IndexSizeType indexSize = *reinterpret_cast<IndexSizeType const*>(mData.data());
    assert(mData.size() >= indexSize * sizeof(IndexElementType) + sizeof(IndexSizeType));
    return indexSize;
  }
  // return the number of elements managed in this container
  size_t getNElements() const
  {
    size_t const offset = getTruthElementArrayOffset();
    assert(mData.size() >= offset);
    assert(((mData.size() - offset) % sizeof(TruthElementType)) == 0);
    return (mData.size() - offset) / sizeof(TruthElementType);
  }

  // get individual "view" container for a given data index
  // the caller can do modifications on this view (such as sorting)
  gsl::span<TruthElement> getLabels(uint dataindex)
  {
    if (dataindex >= getIndexedSize()) {
      return gsl::span<TruthElement>();
    }
    return gsl::span<TruthElement>(&(*this)[getMCTruthHeader(dataindex).index], getSize(dataindex));
  }

  // get individual const "view" container for a given data index
  // the caller can't do modifications on this view
  gsl::span<const TruthElement> getLabels(uint dataindex) const
  {
    if (dataindex >= getIndexedSize()) {
      return gsl::span<const TruthElement>();
    }
    return gsl::span<const TruthElement>(&(*this)[getMCTruthHeader(dataindex).index], getSize(dataindex));
  }

  void clear()
  {
    mData.resize(sizeof(IndexSizeType), 0);
  }

  // add element for a particular dataindex
  // at the moment only strictly consecutive modes are supported
  void addElement(uint dataindex, TruthElement const& element)
  {
    auto indexedSize = getIndexedSize();
    auto nElements = getNElements();
    auto offset = getTruthElementArrayOffset();
    if (dataindex < indexedSize) {
      // look if we have something for this dataindex already
      // must currently be the last one
      if (dataindex != (indexedSize - 1)) {
        throw std::runtime_error("MCTruthContainer: unsupported code path");
      }
      mData.resize(mData.size() + sizeof(TruthElementType));
    } else {
      // assert(dataindex == indexedSize);

      // add empty holes
      int grow = dataindex - indexedSize;
      assert(grow >= 0);
      if (grow < 0) {
        grow = 0;
      }
      grow++;
      mData.resize(mData.size() + grow * sizeof(IndexElementType) + sizeof(TruthElementType));
      auto truthElementsStart = mData.data() + offset;
      memmove(truthElementsStart + grow * sizeof(IndexElementType), truthElementsStart, nElements * sizeof(TruthElementType));
      IndexElementType* fillStart = reinterpret_cast<IndexElementType*>(mData.data() + sizeof(IndexSizeType)) + indexedSize;
      IndexElementType* fillEnd = fillStart + grow;
      for (; fillStart != fillEnd; fillStart++) {
        *fillStart = nElements;
      }
      *reinterpret_cast<IndexSizeType*>(mData.data()) += grow;
    }
    *reinterpret_cast<TruthElementType*>(mData.data() + mData.size() - sizeof(TruthElementType)) = element;
  }

  // convenience interface to add multiple labels at once
  // can use elements of any assignable type or sub-type
  template <typename CompatibleLabel>
  void addElements(uint dataindex, gsl::span<CompatibleLabel> elements)
  {
    static_assert(std::is_same<TruthElement, CompatibleLabel>::value ||
                    std::is_assignable<TruthElement, CompatibleLabel>::value ||
                    std::is_base_of<TruthElement, CompatibleLabel>::value,
                  "Need to add compatible labels");
    for (auto& e : elements) {
      addElement(dataindex, e);
    }
  }

  template <typename CompatibleLabel>
  void addElements(uint dataindex, const std::vector<CompatibleLabel>& v)
  {
    using B = typename std::remove_const<CompatibleLabel>::type;
    auto s = gsl::span<CompatibleLabel>(const_cast<B*>(&v[0]), v.size());
    addElements(dataindex, s);
  }

  // Add element at last position or for a previous index
  // (at random access position).
  // This might be a slow process since data has to be moved internally
  // so this function should be used with care.
  //void addElementRandomAccess(uint dataindex, TruthElement const& element)
  //{
  //  if (dataindex >= mHeaderArray.size()) {
  //    // a new dataindex -> push element at back
  //
  //    // we still forbid to leave holes
  //    assert(dataindex == mHeaderArray.size());
  //
  //    mHeaderArray.resize(dataindex + 1);
  //    mHeaderArray[dataindex] = mTruthArray.size();
  //    mTruthArray.emplace_back(element);
  //  } else {
  //    // if appending at end use fast function
  //    if (dataindex == mHeaderArray.size() - 1) {
  //      addElement(dataindex, element);
  //      return;
  //    }
  //
  //    // existing dataindex
  //    // have to:
  //    // a) move data;
  //    // b) insert new element;
  //    // c) adjust indices of all headers right to this
  //    auto currentindex = mHeaderArray[dataindex].index;
  //    auto lastindex = currentindex + getSize(dataindex);
  //    assert(currentindex >= 0);
  //
  //    // resize truth array
  //    mTruthArray.resize(mTruthArray.size() + 1);
  //    // move data (have to do from right to left)
  //    for (int i = mTruthArray.size() - 1; i > lastindex; --i) {
  //      mTruthArray[i] = mTruthArray[i - 1];
  //    }
  //    // insert new element
  //    mTruthArray[lastindex] = element;
  //
  //    // fix headers
  //    for (uint i = dataindex + 1; i < mHeaderArray.size(); ++i) {
  //      auto oldindex = mHeaderArray[i].index;
  //      mHeaderArray[i].index = (oldindex != -1) ? oldindex + 1 : oldindex;
  //    }
  //  }
  //}

  // merge another container to the back of this one
  //void mergeAtBack(MCTruthContainer<TruthElement> const& other)
  //{
  //  const auto oldtruthsize = mTruthArray.size();
  //  const auto oldheadersize = mHeaderArray.size();
  //
  //  // copy from other
  //  std::copy(other.mHeaderArray.begin(), other.mHeaderArray.end(), std::back_inserter(mHeaderArray));
  //  std::copy(other.mTruthArray.begin(), other.mTruthArray.end(), std::back_inserter(mTruthArray));
  //
  //  // adjust information of newly attached part
  //  for (uint i = oldheadersize; i < mHeaderArray.size(); ++i) {
  //    mHeaderArray[i].index += oldtruthsize;
  //  }
  //}

  ClassDefNV(MCTruthContainer, 2);
}; // end class

} // namespace dataformats
} // namespace o2

#endif
