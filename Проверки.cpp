#include "Библиотека.h"
#include <algorithm>
#include <cassert>
#include <iostream>
#include <map>
#include <random>
#include <stdexcept>

using namespace std;

size_t brute(const u32string& a, const u32string& b, size_t i, size_t j) {
    if (i == a.size()) return b.size() - j;
    if (j == b.size()) return a.size() - i;
    return min({1 + brute(a, b, i + 1, j),
                1 + brute(a, b, i, j + 1),
                (a[i] != b[j]) + brute(a, b, i + 1, j + 1)});
}

void check_table(const Table& t, const map<u32string, vector<Hit>>& ref) {
    assert(t.n == ref.size());
    size_t count = 0;
    for (const auto& bucket : t.a) {
        for (const Word& w : bucket) {
            ++count;
            assert(ref.count(w.text) == 1);
            const auto& expected = ref.at(w.text);
            assert(w.hits.size() == expected.size());
            for (size_t i = 0; i < expected.size(); ++i) {
                assert(w.hits[i].file == expected[i].file);
                assert(w.hits[i].line == expected[i].line);
            }
            assert(find(t, w.text) == &w);
        }
    }
    assert(count == ref.size());
}

int main() {
    assert((words("КОТ, кот! ЁЛКА tree-SEARCH 123") ==
            vector<u32string>{U"кот", U"кот", U"ёлка", U"tree", U"search"}));
    assert(words("123 !!!").empty());
    assert(decode(encode(U"Я ё English 😀")) == U"Я ё English 😀");
    for (string bad : {string("\xFF"), string("\xD0"), string("\xC0\x80"),
                       string("\xED\xA0\x80"), string("\xF4\x90\x80\x80")}) {
        bool caught = false;
        try { decode(bad); } catch (const runtime_error&) { caught = true; }
        assert(caught);
    }

    Table t;
    map<u32string, vector<Hit>> ref;
    mt19937 gen(20261117);
    for (int i = 0; i < 12000; ++i) {
        u32string s;
        size_t len = 1 + gen() % 10;
        for (size_t j = 0; j < len; ++j) s += U'a' + gen() % 6;
        Hit h{static_cast<size_t>(i % 11), static_cast<size_t>(i + 1)};
        add(t, s, h);
        ref[s].push_back(h);
    }
    check_table(t, ref);
    assert(find(t, U"несуществующее") == nullptr);
    rehash(t, 1);
    check_table(t, ref);
    rehash(t, 32768);
    check_table(t, ref);

    assert(edit_distance(U"таблца", U"таблица") == 1);
    assert(edit_distance(U"кот", U"кит") == 1);
    assert(edit_distance(U"", U"ёлка") == 4);
    assert(edit_distance(U"кот", U"кот") == 0);
    assert(edit_distance(U"кот", U"кто") == 2);
    for (int i = 0; i < 500; ++i) {
        u32string a, b;
        size_t n = gen() % 5, m = gen() % 5;
        for (size_t j = 0; j < n; ++j) a += U'а' + gen() % 3;
        for (size_t j = 0; j < m; ++j) b += U'а' + gen() % 3;
        assert(edit_distance(a, b) == brute(a, b, 0, 0));
    }

    Table small;
    vector<u32string> dict = {U"кот", U"кит", U"коты", U"таблица", U"ёлка", U"котик"};
    for (const auto& w : dict) add(small, w, Hit{0, 1});
    for (const auto& q : dict) {
        for (size_t limit = 0; limit <= 2; ++limit) {
            auto found = similar(small, q, limit);
            vector<Match> expected;
            for (const auto& w : dict) {
                size_t d = brute(q, w, 0, 0);
                if (d <= limit) expected.push_back(Match{w, d});
            }
            sort(expected.begin(), expected.end(), [](const Match& a, const Match& b) {
                if (a.dist != b.dist) return a.dist < b.dist;
                return a.word < b.word;
            });
            assert(found.size() == expected.size());
            for (size_t i = 0; i < found.size(); ++i) {
                assert(found[i].word == expected[i].word && found[i].dist == expected[i].dist);
            }
        }
    }
    cout << "OK: 12000 insertions, collisions, rehashing, UTF-8, 500 distance cases, fuzzy search.\n";
}
