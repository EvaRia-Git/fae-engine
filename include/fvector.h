#pragma once
#include <cstdint>

namespace fae 
{
	// Use fastest integer type for indexing
	using index = std::uint_fast32_t;

	// Expandable array type
	// Provides quick insertions and deletions
	template <typename T>
	class fVector
	{
	public:
		// Fields
		index length;	// Dynamic size
		index capacity;	// Memory buffer
		T * at;			// Starting address

		//
		// Constructors
		//

		//
		// Default Constructor
		fVector() : length(0), capacity(0), at(nullptr)
		{
		}

		//
		// Copy Constructor
		fVector(fVector const & source)
		{
			// Copy fields
			length = source.length;
			capacity = source.capacity;

			// Copy existing elements
			if (capacity > 0) {
				at = new T[capacity];
				for (index i = 0; i < source.length; ++i) {
					at[i] = source.at[i];
				}
			}
			else { // Source is empty
				at = nullptr;
			}
		}

		//
		// Destructor
		~fVector()
		{
			if (at != nullptr) {
				delete[] at;
			}
		}

		//
		// Operators
		//

		//
		// Copy Assignment
		fVector & operator = (fVector const & source)
		{
			// Replace current data
			if (at != nullptr) {
				delete[] at;
			}

			// Copy fields
			length = source.length;
			capacity = source.capacity;

			// Copy existing elements
			if (capacity > 0) {
				at = new T[capacity];
				for (index i = 0; i < source.length; ++i) {
					at[i] = source.at[i];
				}
			}
			else { // Source is empty
				at = nullptr;
			}

			return *this;
		}

		//
		// Read Access
		T const & operator[] (index const & i) const
		{
			return at[i];
		}

		//
		// Write Access
		T & operator[] (index const & i)
		{
			return at[i];
		}

		//
		// Methods
		//

		//
		// Allocates more memory
		void expand()
		{
			// Default capacity of 4
			if (capacity == 0) {
				capacity = 4;
				at = new T[4];
			}
			else { // Contents exist

				// Recreate buffer with double capacity
				capacity *= 2;
				T * temp = new T[capacity];

				// Copy old contents and free old memory
				for (index i = 0; i < length; ++i) {
					temp[i] = at[i];
				}
				// Replace buffer
				delete[] at;
				at = temp;
			}
		}

		//
		// Frees all memory
		void release()
		{
			if (at != nullptr) {
				length = 0;
				capacity = 0;
				delete[] at;
				at = nullptr;
			}
		}

		//
		// Adds an element to the end
		void push(T const & value)
		{
			if (length == capacity) {
				expand();
			}
			at[length] = value;
			++length;
		}

		//
		// Removes an element from the end
		void pop()
		{
			if (length != 0) {
				--length;
			}
		}

		//
		// Removes all elements
		void clear()
		{
			length = 0;
		}

		//
		// Erases an element at specified index
		void erase(index const & target)
		{
			pop();
			// Shift out
			for (index i = target; i < length; ++i) {
				at[i] = at[i + 1];
			}
		}

		//
		// Inserts an element at specified index
		void insert(index const & dest, T const & value)
		{
			// Expand if necessary
			if (length == capacity) {
				expand();
			}
			// Shift over
			for (index i = length; i > dest; --i) {
				at[i] = at[i - 1];
			}
			// Copy in
			at[dest] = value;
			++length;
		}

		//
		// Concatenates another array to this
		void concatenate(fVector const & append)
		{
			// Recalculate length
			index newLength = length + append.length;

			// Cache index
			index i;

			// Resize if needed
			if (capacity < newLength) {
				while (capacity < newLength) {
					capacity *= 2;
				}
				// Use only one reallocation
				T * temp = new T[capacity];

				// Copy into new buffer
				for (i = 0; i < length; ++i) {
					temp[i] = at[i];
				}

				// Replace old buffer
				delete[] at;
				at = temp;
			}
			// Copy append into array
			for (i = length; i < newLength; ++i) {
				at[i] = append.at[i];
			}
			// Update length
			length = newLength;
		}
	};
}
