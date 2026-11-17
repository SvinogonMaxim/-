#ifndef SEARCH_LIBRARY_H
#define SEARCH_LIBRARY_H

#include <cstddef>
#include <cstdint>
#include <list>
#include <string>
#include <vector>

struct Hit {
    std::size_t file;
    std::size_t line;
};

struct Word {
    std::u32string text;
    std::vector<Hit> hits;
};

struct Table {
    std::vector<std::list<Word>> a;
    std::size_t n = 0;
    Table();
};

std::u32string decode(const std::string& s);
std::string encode(const std::u32string& s);
std::vector<std::u32string> words(const std::string& s);

std::uint64_t hash_word(const std::u32string& s);
const Word* find(const Table& t, const std::u32string& s);
void rehash(Table& t, std::size_t m);
void add(Table& t, const std::u32string& s, Hit hit);

std::size_t edit_distance(const std::u32string& a, const std::u32string& b);

struct Match {
    std::u32string word;
    std::size_t dist;
};

std::vector<Match> similar(const Table& t, const std::u32string& s,
                         std::size_t limit);

#endif
