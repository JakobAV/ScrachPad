#include "Sort.hpp"
#include <stdio.h>
#include <chrono>
#include <random>

using namespace CommonUtilities;

#define TimeLim(alogrithm, vector, limit) do{\
if(limit == 0 || vector.size() <= limit){\
double timeElapsed = TimeSort(alogrithm, vector);\
printf("%-15s %zu: %lf s\n", #alogrithm, vector.size(), timeElapsed); }\
else{printf("%-15s %zu: %s\n", #alogrithm, vector.size(), "Fuck off time");}}while(false);

#define Time(alogrithm, vector) TimeLim(alogrithm, vector, 0)

float Random()
{
	static std::random_device seed;
	static std::mt19937 engine(seed());
	static std::uniform_real_distribution<float> oneToZero(0.0f, 1.0f);

	return oneToZero(engine);
}

template<class T>
T RandomRange(T min, T max)
{
	T range = max - min;

	T result = min + static_cast<T>(Random() * range);
	return result;
}

void FillRandom(std::vector<int>& vector, int min = 0, int max = 10)
{
	for (size_t i = 0; i < vector.size(); ++i)
	{
		vector[i] = RandomRange(min, max);
	}
}

double TimeSort(void (*sort)(std::vector<int>&), std::vector<int>& unsorted)
{
	auto toSort = unsorted;
	auto start = std::chrono::high_resolution_clock::now();
	sort(toSort);
	auto end = std::chrono::high_resolution_clock::now();
	double timeElapsed = std::chrono::duration<double>(end - start).count();
	return timeElapsed;
}

void TimeAllSorts(size_t count)
{
	std::vector<int> unsorted(count);
	FillRandom(unsorted);
	printf("UnSorted arrays:\n");
	TimeLim(BubbleSort, unsorted, 50000);
	TimeLim(MergeSort, unsorted, 100000);
	TimeLim(SelectionSort, unsorted, 100000);
	Time(QuickSort, unsorted);
	Time(Radix256Sort, unsorted);
	Radix256Sort(unsorted);
	printf("Sorted arrays:\n");
	Time(BubbleSort, unsorted);
	TimeLim(MergeSort, unsorted, 100000);
	TimeLim(SelectionSort, unsorted, 100000);
	Time(QuickSort, unsorted);
	Time(Radix256Sort, unsorted);
	printf("\n");
}

void SortTiming()
{
	TimeAllSorts(10);
	TimeAllSorts(1000);
	TimeAllSorts(100000);
	TimeAllSorts(10000000);
	TimeAllSorts(1000000000);
	system("pause");
}