#pragma once
#include <vector>

namespace CommonUtilities
{
	template <class T>
	void SelectionSort(std::vector<T>& aVector)
	{
		for (int i = 0; i < aVector.size(); ++i)
		{
			int min = i;
			for (int j = i + 1; j < aVector.size(); ++j)
			{
				if (aVector[j] < aVector[min])
				{
					min = j;
				}
			}
			T temp = aVector[i];
			aVector[i] = aVector[min];
			aVector[min] = temp;
		}
	}

	template <class T>
	void BubbleSort(std::vector<T>& aVector)
	{
		bool swapped;
		do
		{
			swapped = false;
			for (int i = 1; i < aVector.size(); ++i)
			{
				if (aVector[i] < aVector[i - 1])
				{
					T temp = aVector[i - 1];
					aVector[i - 1] = aVector[i];
					aVector[i] = temp;
					swapped = true;
				}
			}
		} while (swapped);
	}

	template <class T>
	void QuickSort(std::vector<T>& aVector, int aLow, int aHigh)
	{
		if (aHigh < 0 || aLow < 0 || aLow >= aHigh)
		{
			return;
		}
		int j = 0;
		{
			int i = aLow - 1;
			j = aHigh + 1;
			int mid = aLow + (aHigh - aLow) / 2;
			T v = aVector[mid];
			for (;;)
			{
				do {} while (aVector[++i] < v);
				do {} while (v < aVector[--j]);

				if (i >= j)
				{
					break;
				}
				T temp = aVector[i];
				aVector[i] = aVector[j];
				aVector[j] = temp;
			}
		}

		QuickSort(aVector, aLow, j);
		QuickSort(aVector, j + 1, aHigh);
	}

	template <class T>
	void QuickSort(std::vector<T>& aVector)
	{
		QuickSort(aVector, 0, static_cast<int>(aVector.size()) - 1);
	}

	template <class T>
	void MergeSort(std::vector<T>& aVector, std::vector<T>& workArea, int aLow, int aHigh)
	{
		if (aHigh <= aLow)
		{
			return;
		}

		int mid = aLow + (aHigh - aLow) / 2;
		MergeSort(aVector, workArea, aLow, mid);
		MergeSort(aVector, workArea, mid + 1, aHigh);

		int i = aLow;
		int j = mid + 1;
		for (int k = aLow; k <= aHigh; ++k)
		{
			workArea[k] = aVector[k];
		}
		for (int k = aLow; k <= aHigh; ++k)
		{
			if (i > mid)
			{
				aVector[k] = workArea[j];
				++j;
			}
			else if (j > aHigh)
			{
				aVector[k] = workArea[i];
				++i;
			}
			else if (workArea[j] < workArea[i])
			{
				aVector[k] = workArea[j];
				++j;
			}
			else
			{
				aVector[k] = workArea[i];
				++i;
			}
		}
	}

	template <class T>
	void MergeSort(std::vector<T>& aVector)
	{
		std::vector<T> workArea = aVector;
		MergeSort(aVector, workArea, 0, static_cast<int>(aVector.size()) - 1);
		workArea;
	}

	// NOTE: If you want a compex data type to work with Radix256Sort,
	// you will need to implement an operator overide for >> that creates an integer that represents the sort order value of the item,
	// and then does the do the normal >> operator with that number and the other operand.
	template <class T>
	void Radix256Sort(std::vector<T>& aVector)
	{
		int n = static_cast<int>(aVector.size());
		T* count = new T[256];
		std::vector<T> workArea = aVector;
		int passes = sizeof(T);
		for (int pass = 0; pass < passes; ++pass)
		{
			for (int i = 0; i < 256; ++i)
			{
				count[i] = 0;
			}
			for (int i = 0; i < n; ++i)
			{
				++count[(aVector[i] >> (8 * pass)) & 0xFF]; // Same as aVector[i] % 256
			}
			// Prefix sum
			for (int i = 1; i < 256; ++i)
			{
				count[i] += count[i - 1];
			}
			// Place items at the correct index for pass
			for (int i = n - 1; i >= 0; --i)
			{
				int index = --count[(aVector[i] >> (8 * pass)) & 0xFF];
				workArea[index] = aVector[i];
			}
			aVector.swap(workArea);
		}
		// If the amount of passes where odd, we need to swap one extra time
		if (passes & 1)
		{
			aVector.swap(workArea);
		}
		delete[] count;
	}
}
