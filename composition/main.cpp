#include <iostream>

bool NextPermutation(size_t* permutation, const size_t* array_of_sizes,
                     size_t size) {
  permutation[size - 1] += 1;
  for (size_t index = size - 1; index > 0; --index) {
    if (permutation[index] == array_of_sizes[index]) {
      permutation[index - 1] += 1;
      permutation[index] = 0;
    }
  }
  return permutation[0] != array_of_sizes[0];
}

bool IsPermValid(const size_t* permutation, size_t max_size, size_t perm_size,
                 bool* is_repeated) {
  for (size_t index = 0; index < perm_size; ++index) {
    if (is_repeated[permutation[index]]) {
      for (size_t inner_index = 0; inner_index < max_size; ++inner_index) {
        is_repeated[inner_index] = false;
      }
      return false;
    }
    is_repeated[permutation[index]] = true;
  }
  for (size_t index = 0; index < max_size; ++index) {
    is_repeated[index] = false;
  }
  return true;
}

void FillArrays(size_t num_of_arrays, const size_t* array_of_sizes,
                int** array_of_arrays) {
  for (size_t first_counter = 0; first_counter < num_of_arrays;
       ++first_counter) {
    for (size_t second_counter = 0;
         second_counter < array_of_sizes[first_counter]; ++second_counter) {
      std::cin >> array_of_arrays[first_counter][second_counter];
    }
  }
}

long long GetSumOfProducts(int** array_of_arrays, size_t* permutation,
                           size_t* array_of_sizes, size_t num_of_arrays,
                           size_t max_array_size) {
  long long sum = 0;
  if (num_of_arrays == 1) {
    sum += array_of_arrays[0][0];
  }
  bool* is_repeated = new bool[max_array_size]{false};
  long long intermediate_sum = 1;
  while (NextPermutation(permutation, array_of_sizes, num_of_arrays)) {
    if (IsPermValid(permutation, max_array_size, num_of_arrays, is_repeated)) {
      for (size_t index = 0; index < num_of_arrays; ++index) {
        intermediate_sum *= array_of_arrays[index][permutation[index]];
      }
      sum += intermediate_sum;
      intermediate_sum = 1;
    }
  }
  delete[] is_repeated;
  return sum;
}

void Solve(int argc, char** argv) {
  size_t num_of_arrays = argc - 1;
  int** array_of_arrays = new int* [num_of_arrays] { nullptr };
  size_t* array_of_sizes = new size_t[num_of_arrays];
  for (size_t index = 0; index < num_of_arrays; ++index) {
    array_of_sizes[index] = std::stoi(argv[index + 1]);
    array_of_arrays[index] = new int[array_of_sizes[index]];
  }
  FillArrays(num_of_arrays, array_of_sizes, array_of_arrays);
  size_t max_array_size = 0;
  for (size_t index = 0; index < num_of_arrays; ++index) {
    if (array_of_sizes[index] > max_array_size) {
      max_array_size = array_of_sizes[index];
    }
  }
  size_t* permutation = new size_t[num_of_arrays]{0};
  std::cout << GetSumOfProducts(array_of_arrays, permutation, array_of_sizes,
                                num_of_arrays, max_array_size);
  delete[] array_of_sizes;
  for (size_t index = 0; index < num_of_arrays; ++index) {
    delete[] array_of_arrays[index];
  }
  delete[] array_of_arrays;
  delete[] permutation;
}

int main(int argc, char** argv) {
  Solve(argc, argv);
  return 0;
}
