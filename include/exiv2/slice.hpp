// ********************************************************* -*- C++ -*-
/*
 * Copyright (C) 2004-2018 Exiv2 maintainers
 *
 * This program is part of the Exiv2 distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, 5th Floor, Boston, MA 02110-1301 USA.
 */
/*!
  @file    slice.hpp
  @brief   Simple implementation of slices (=views) for STL containers and C-arrays
  @author  Dan Čermák (D4N)
           <a href="mailto:dan.cermak@cgc-instruments.com">dan.cermak@cgc-instruments.com</a>
  @date    30-March-18, D4N: created
 */

#ifndef EXIV2_INCLUDE_SLICE_HPP
#define EXIV2_INCLUDE_SLICE_HPP

#include <cassert>
#include <cstddef>
#include <iterator>
#include <stdexcept>
#include <type_traits>

namespace Exiv2
{
#ifndef PARSED_BY_DOXYGEN
    namespace Internal
    {
        /*!
         * Common base class of all slice implementations.
         *
         * Implements only the most basic functions, which do not require any
         * knowledge about the stored data.
         */
        struct SliceBase
        {
            inline SliceBase(size_t begin, size_t end) : begin_(begin), end_(end)
            {
                if (begin >= end) {
                    throw std::out_of_range("Begin must be smaller than end");
                }
            }

            /*!
             * Return the number of elements in the slice.
             */
            inline size_t size() const noexcept
            {
                // cannot underflow, as we know that begin < end
                return end_ - begin_;
            }

        protected:
            /*!
             * Throw an exception when index is too large.
             *
             * @throw std::out_of_range when `index` will access an element
             *     outside of the slice
             */
            inline void rangeCheck(size_t index) const
            {
                if (index >= size()) {
                    throw std::out_of_range("Index outside of the slice");
                }
            }

            /*!
             * Modify begin & end in-place so that they are the correct bounds
             * of the new sub-slice.
             *
             * This is a helper function for the subSlice() member functions: it
             * calculates the bounds with respect to the original
             * container/pointer.
             *
             * @param[in,out] begin, end new bounds of a subSlice, this function
             *     adds the current `begin_` and `end_` to these safely.
             *
             * @throw std::out_of_range when `begin` or `end` will access an
             *     element outside of the slice
             */
            inline void calculateSubSliceBounds(size_t& begin, size_t& end) const
            {
                // end == size() is a legal value, since end is the first element beyond the slice
                // end == 0 is not a legal value
                if ((begin >= size()) || (end > size())) {
                    throw std::out_of_range("sub-Slice index out of range");
                }

                // additions are safe, begin and end are smaller than size()
                begin += this->begin_;
                end += this->begin_;

                if (end > this->end_) {
                    throw std::out_of_range("sub-Slice index out of range");
                }
            }

            /*!
             * lower and upper bounds of the slice with respect to the
             * container/array stored in storage_
             */
            const size_t begin_, end_;
        };
    }   // namespace Internal
#endif  // PARSED_BY_DOXYGEN

    /*!
     * @brief Slice (= view) for STL containers.
     *
     * This is a very simple implementation of slices (i.e. views of sub-arrays)
     * for STL containers that support O(1) element access and random access
     * iterators (like std::vector, std::array and std::string).
     *
     * A slice represents the semi-open interval [begin, end) and provides a
     * (mutable) view, it does however not own the data! It can be used to
     * conveniently pass parts of containers into functions without having to use
     * iterators or offsets.
     *
     * In contrast to C++20's std::span<T> it is impossible to read beyond the
     * container's bounds and unchecked access is not-possible (by design).
     *
     * Example usage:
     * ~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
     * std::vector<int> vec = {0, 1, 2, 3, 4};
     * slice<std::vector<int> > one_two(vec, 1, 3);
     * assert(one_two.size() == 2);
     * assert(one_two.at(0) == 1 && one_two.at(1) == 2);
     * // mutate the contents:
     * one_two.at(0) *= 2;
     * one_two.at(1) *= 3;
     * assert(one_two.at(0) == 2 && one_two.at(1) == 6);
     * ~~~~~~~~~~~~~~~~~~~~~~~~~
     *
     * Slices also offer access via iterators of the same type as the underlying
     * container, so that they can be used in a comparable fashion:
     * ~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
     * std::vector<int> vec = {0, 1, 2, 3, 4};
     * slice<std::vector<int>> three_four(vec, 3, 5);
     * assert(*three_four.begin() == 3 && *three_four.end() == 4);
     * // this prints:
     * // 3
     * // 4
     * for (const auto & elem : three_four) {
     *     std::cout << elem << std::endl;
     * }
     * ~~~~~~~~~~~~~~~~~~~~~~~~~
     *
     * @tparam container A STL container type, like vector or array. Must support
     * array-like access via the `at()` method.
     */
    template <typename container>
    struct Slice : Internal::SliceBase
    {
        typedef typename container::iterator iterator;

        typedef typename container::const_iterator const_iterator;

        typedef typename std::remove_cv<typename container::value_type>::type value_type;

        /*!
         * @brief Construct a slice of the container `cont` starting at `begin`
         * (including) and ending before `end`.
         *
         * @param[in] cont Reference to the container
         * @param[in] begin First element of the slice.
         * @param[in] end First element beyond the slice.
         *
         * @throws std::out_of_range For invalid slice bounds: when end is not
         * larger than begin or when the slice's bounds are larger than the
         * container's size.
         *
         * Please note that due to the requirement that `end` must be larger
         * than `begin` (they cannot be equal) it is impossible to construct a
         * slice with zero length.
         */
        Slice(container& cont, size_t begin, size_t end) : Internal::SliceBase(begin, end), cont_(cont)
        {
            if (end > cont.size()) {
                throw std::out_of_range("End must not be larger than the container.");
            }
        }

        /*!
         * Obtain a constant reference to the element with the given `index` in
         * the container.
         *
         * @throw whatever container::at() throws
         */
        const value_type& at(size_t index) const
        {
            rangeCheck(index);
            // we know: begin_ < end <= size() <= SIZE_T_MAX
            // and: index < end - begin
            // thus: index + begin < end <= SIZE_T_MAX
            // => no overflow is possible
            return cont_.at(begin_ + index);
        }

        /*!
         * Obtain a mutable reference to the element with the given `index` in
         * the container.
         * Please note that this function is unavailable in constant slices.
         *
         * @throw whatever container::at() throws.
         */
#ifdef PARSED_BY_DOXYGEN
        value_type&
#else
        template <typename Dummy = container>
        typename std::enable_if<!std::is_const<Dummy>::value, value_type>::type&
#endif
        at(size_t index)
        {
            rangeCheck(index);
            // we know: begin_ < end <= size() <= SIZE_T_MAX
            // and: index < end - begin
            // thus: index + begin < end <= SIZE_T_MAX
            // => no overflow is possible
            return cont_.at(begin_ + index);
        }

#ifndef PARSED_BY_DOXYGEN
#define RETURN_ITERATOR(getter_method, offset) \
    auto iter = cont_.getter_method();         \
    std::advance(iter, offset);                \
    return iter
#endif

        const_iterator cbegin() const
        {
            RETURN_ITERATOR(cbegin, this->begin_);
        }

#ifdef PARSED_BY_DOXYGEN
        iterator
#else
        template <typename Dummy = container>
        typename std::enable_if<!std::is_const<Dummy>::value, iterator>::type
#endif
        begin()
        {
            RETURN_ITERATOR(begin, this->begin_);
        }

        const_iterator begin() const
        {
            RETURN_ITERATOR(begin, this->begin_);
        }

        const_iterator cend() const
        {
            RETURN_ITERATOR(cbegin, this->end_);
        }

#ifdef PARSED_BY_DOXYGEN
        iterator
#else
        template <typename Dummy = container>
        typename std::enable_if<!std::is_const<Dummy>::value, iterator>::type
#endif
        end()
        {
            RETURN_ITERATOR(begin, this->end_);
        }

        const_iterator end() const
        {
            RETURN_ITERATOR(begin, this->end_);
        }

#ifndef PARSED_BY_DOXYGEN
#undef RETURN_ITERATOR
#endif

        /*!
         * Construct a sub-slice of this slice with the given bounds. The bounds
         * are evaluated with respect to the current slice.
         *
         * Note: the non-const overload is only available when the container is
         * also not const.
         *
         * @param[in] begin  First element in the new slice.
         * @param[in] end  First element beyond the new slice.
         *
         * @throw std::out_of_range when begin or end are invalid
         */
#ifdef PARSED_BY_DOXYGEN
        Slice<container>
#else
        template <typename Dummy = container>
        typename std::enable_if<!std::is_const<Dummy>::value, Slice<container> >::type
#endif
        subSlice(size_t begin, size_t end)
        {
            this->calculateSubSliceBounds(begin, end);
            return Slice(cont_, begin, end);
        }

        /*!
         * Constructs a new constant subSlice. Behaves otherwise exactly like
         * the non-const version.
         */
        Slice<container> subSlice(size_t begin, size_t end) const
        {
            this->calculateSubSliceBounds(begin, end);
            return Slice<container>(cont_, begin, end);
        }

    private:
        container& cont_;
    };

    /*!
     * Specialization of slices for constant C-arrays.
     *
     * These have exactly the same interface as the slices for STL-containers,
     * with the *crucial* exception, that the slice's constructor *cannot* make
     * a proper bounds check! It can only verify that you didn't accidentally
     * swap begin and end!
     */
    template <typename T>
    struct Slice<T*> : Internal::SliceBase
    {
        typedef typename std::remove_cv<T>::type value_type;

        typedef typename std::conditional<std::is_const<T>::value, typename std::add_const<value_type>::type*,
                                          value_type*>::type iterator;

        typedef const typename std::remove_cv<T>::type* const_iterator;

        /*!
         * Constructor.
         *
         * @param[in] ptr  C-array of which a slice should be constructed. Must
         *     not be a null pointer.
         * @param[in] begin  Index of the first element in the slice.
         * @param[in] end  Index of the first element that is no longer in the
         *     slice.
         *
         * Please note that the constructor has no way how to verify that
         * `begin` and `end` are not out of bounds of the provided array!
         */
        Slice(T* ptr, size_t begin, size_t end) : Internal::SliceBase(begin, end), ptr_(ptr)
        {
            if (ptr == nullptr) {
                throw std::invalid_argument("ptr must not be null.");
            }
        }

        const value_type& at(size_t index) const
        {
            rangeCheck(index);
            // we know: begin_ < end <= size() <= SIZE_T_MAX
            // and: index < end - begin
            // thus: index + begin < end <= SIZE_T_MAX
            // => no overflow is possible
            return ptr_[begin_ + index];
        }

#ifdef PARSED_BY_DOXYGEN
        value_type&
#else
        template <typename Dummy = T>
        typename std::enable_if<!std::is_const<Dummy>::value, value_type>::type&
#endif
        at(size_t index)
        {
            rangeCheck(index);
            // we know: begin_ < end <= size() <= SIZE_T_MAX
            // and: index < end - begin
            // thus: index + begin < end <= SIZE_T_MAX
            // => no overflow is possible
            return ptr_[begin_ + index];
        }

        const_iterator cbegin() const noexcept
        {
            return ptr_ + begin_;
        }

#ifdef PARSED_BY_DOXYGEN
        iterator
#else
        template <typename Dummy = T>
        typename std::enable_if<!std::is_const<Dummy>::value, iterator>::type
#endif
        begin() noexcept
        {
            return ptr_ + begin_;
        }

        const_iterator begin() const noexcept
        {
            return ptr_ + begin_;
        }

        const_iterator cend() const noexcept
        {
            return ptr_ + end_;
        }

#ifdef PARSED_BY_DOXYGEN
        iterator
#else
        template <typename Dummy = T>
        typename std::enable_if<!std::is_const<Dummy>::value, iterator>::type
#endif
        end() noexcept
        {
            return ptr_ + end_;
        }

        const_iterator end() const noexcept
        {
            return ptr_ + end_;
        }

#ifdef PARSED_BY_DOXYGEN
        Slice<T*>
#else
        template <typename Dummy = T>
        typename std::enable_if<!std::is_const<Dummy>::value, Slice<T*> >::type
#endif
        subSlice(size_t begin, size_t end)
        {
            this->calculateSubSliceBounds(begin, end);
            return Slice<T*>(ptr_, begin, end);
        }

        Slice<const T*> subSlice(size_t begin, size_t end) const
        {
            this->calculateSubSliceBounds(begin, end);
            return Slice<const T*>(ptr_, begin, end);
        }

    private:
        T* ptr_;
    };

    /*!
     * @brief Return a new slice with the given bounds.
     *
     * Convenience wrapper around the slice's constructor for automatic template
     * parameter deduction.
     */
    template <typename T>
    inline Slice<T> makeSlice(T& cont, size_t begin, size_t end)
    {
        return Slice<T>(cont, begin, end);
    }

    /*!
     * Overload of makeSlice for slices of C-arrays.
     */
    template <typename T>
    inline Slice<T*> makeSlice(T* ptr, size_t begin, size_t end)
    {
        return Slice<T*>(ptr, begin, end);
    }

    /*!
     * @brief Return a new slice spanning the whole container.
     */
    template <typename container>
    inline Slice<container> makeSlice(container& cont)
    {
        return Slice<container>(cont, 0, cont.size());
    }

    /*!
     * @brief Return a new slice spanning from begin until the end of the
     * container.
     */
    template <typename container>
    inline Slice<container> makeSliceFrom(container& cont, size_t begin)
    {
        return Slice<container>(cont, begin, cont.size());
    }

    /*!
     * @brief Return a new slice spanning until `end`.
     */
    template <typename container>
    inline Slice<container> makeSliceUntil(container& cont, size_t end)
    {
        return Slice<container>(cont, 0, end);
    }

    /*!
     * Overload of makeSliceUntil for pointer based slices.
     */
    template <typename T>
    inline Slice<T*> makeSliceUntil(T* ptr, size_t end)
    {
        return Slice<T*>(ptr, 0, end);
    }

}  // namespace Exiv2

#endif /* EXIV2_INCLUDE_SLICE_HPP */
