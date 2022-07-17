#include <algorithm>
#include <cassert>
#include <cstdio>
#include <iostream>
#include <memory>
#include <random>
#include <stdexcept>
#include <stdint.h>
#include <vector>

using namespace std;

default_random_engine defEngine;
uniform_int_distribution<int> intDistro(0, 100);

struct Record {
  Record() { m_timestamp = intDistro(defEngine); }
  inline uint64_t timestamp() const { return m_timestamp; }

  // inline bool operator()(const unique_ptr<Record> &r1,
  //                        const unique_ptr<Record> &r2) {
  //   return r1->timestamp() < r2->timestamp();
  // }

  void set_timestamp(uint64_t timestamp) { m_timestamp = timestamp; }

private:
  uint64_t m_timestamp;
};

struct Reader {
  Reader() { m_iterator = m_records.begin(); };

  unique_ptr<Record> next_record() {
    if (m_iterator != m_records.end()) {
      auto &&record = std::move(*m_iterator);
      ++m_iterator;
      return std::forward<unique_ptr<Record>>(record);
    }

    return nullptr;
  };
  void feed(vector<unique_ptr<Record>> &records) {
    m_records = std::move(records);
    m_iterator = m_records.begin();
  }

private:
  vector<unique_ptr<Record>> m_records;
  std::vector<unique_ptr<Record>>::iterator m_iterator;
};

template <typename T> void generate(vector<unique_ptr<T>> &v, size_t size) {

  for (size_t i = 0; i < size; i++) {
    v.push_back(std::move(std::make_unique<T>()));
  }
}

void print_vector(vector<unique_ptr<Record>> &v) {
  cout << "data=";
  for (auto &i : v) {
    cout << " " << i->timestamp();
  }
  cout << endl;
}

int main(int argc, const char **argv) {
  bool no_records = false;
  constexpr size_t nr_readers = 10;
  constexpr size_t nr_records = 3;

  int64_t diff_timestamp = 0;
  int64_t max_distance_timestamp;
  size_t max_distance = 1;

  // Assumed sorted Records will be the most frequently used
  vector<unique_ptr<Record>>::iterator position_to_insert;
  vector<unique_ptr<Record>> tmp_vec_records;
  vector<unique_ptr<Record>> sorted_records;

  auto cmp_records = [](const unique_ptr<Record> &r1,
                        const unique_ptr<Record> &r2) {
    return r1->timestamp() < r2->timestamp();
  };

  // Generate Readers and Records
  vector<unique_ptr<Reader>> readers;
  generate<Reader>(readers, nr_readers);
  for (auto &reader : readers) {
    vector<unique_ptr<Record>> records;

    generate<Record>(records, nr_records);
    print_vector(records);
    reader.get()->feed(records);
  }

  // Get one Record from each Reader and place to tmp vector.
  // "Move" Records to sorted vector.

  sorted_records.reserve(
      nr_readers * nr_records); // reserve to protect from memory reallocation

  do {
    no_records = true;
    max_distance_timestamp = 0;

    // take records from readers
    for (auto &reader : readers) // O(N)
    {
      auto record = reader->next_record();
      if (record != nullptr) {
        no_records = false;
        tmp_vec_records.push_back(std::move(record));
      }
    }

    for (auto &&record : tmp_vec_records) {
      // With high probability new records will have greater timestamp then the
      // last record of sorted vector.
      // Checking if vector is not empty is not needed if sorted records vector
      // will be initialized with a Record upfront.
      if (!sorted_records.empty()) {
        diff_timestamp =
            record->timestamp() - sorted_records.back()->timestamp();
      }
      if (diff_timestamp >= 0) {
        // right assumption so just append new record
        // complexity is O(1)
        sorted_records.push_back(std::move(record));
        continue;
      }

      // Trivial prediction based on position of Record with oldest timestamp.
      max_distance_timestamp =
          (*(sorted_records.end() - max_distance))->timestamp();
      if (diff_timestamp < 0 && abs(diff_timestamp) <= max_distance_timestamp) {
        // Find record in range of the oldest record.
        // Complexity is O(max_distance) and much less then O(N).
        // max_distance is number of records to the oldest inserted during
        // current iteration.
        position_to_insert =
            std::upper_bound(sorted_records.end() - max_distance,
                             sorted_records.end(), record, cmp_records);
      } else {
        // Prediction failed - we need search in whole sorted vector.
        // Time complexity is O(log N)
        position_to_insert = std::lower_bound(
            sorted_records.begin(), sorted_records.end(), record, cmp_records);
      }

      // Complexity is O(M) where is nr elements to move (and M->N)
      sorted_records.insert(position_to_insert, std::move(record));

      // Try to predict the range of incoming search in sorted vector.
      size_t current_record_distance =
          sorted_records.end() - position_to_insert;
      max_distance = std::max(max_distance, current_record_distance);
    }

    tmp_vec_records.clear();
  } while (!no_records);

  print_vector(sorted_records);

  return 0;
}