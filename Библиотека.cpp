#include "Библиотека.h"
#include <algorithm>
#include <stdexcept>

using namespace std;

u32string decode(const string& s) {
    u32string out;
    for (size_t i = 0; i < s.size();) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        size_t len;
        char32_t x;
        if (c < 0x80) { len = 1; x = c; }
        else if (c >= 0xC2 && c <= 0xDF) { len = 2; x = c & 0x1F; }
        else if (c >= 0xE0 && c <= 0xEF) { len = 3; x = c & 0x0F; }
        else if (c >= 0xF0 && c <= 0xF4) { len = 4; x = c & 0x07; }
        else throw runtime_error("Файл или ввод не является корректным UTF-8.");
        if (s.size() - i < len) throw runtime_error("Оборванный символ UTF-8.");
        for (size_t j = 1; j < len; ++j) {
            unsigned char d = static_cast<unsigned char>(s[i + j]);
            if ((d & 0xC0) != 0x80) throw runtime_error("Некорректный символ UTF-8.");
            x = (x << 6) | (d & 0x3F);
        }
        if ((len == 2 && x < 0x80) || (len == 3 && x < 0x800) ||
            (len == 4 && x < 0x10000) || x > 0x10FFFF ||
            (x >= 0xD800 && x <= 0xDFFF)) {
            throw runtime_error("Недопустимая последовательность UTF-8.");
        }
        out.push_back(x);
        i += len;
    }
    return out;
}

string encode(const u32string& s) {
    string out;
    for (char32_t x : s) {
        if (x < 0x80) out += static_cast<char>(x);
        else if (x < 0x800) {
            out += static_cast<char>(0xC0 | (x >> 6));
            out += static_cast<char>(0x80 | (x & 0x3F));
        } else if (x < 0x10000) {
            out += static_cast<char>(0xE0 | (x >> 12));
            out += static_cast<char>(0x80 | ((x >> 6) & 0x3F));
            out += static_cast<char>(0x80 | (x & 0x3F));
        } else {
            out += static_cast<char>(0xF0 | (x >> 18));
            out += static_cast<char>(0x80 | ((x >> 12) & 0x3F));
            out += static_cast<char>(0x80 | ((x >> 6) & 0x3F));
            out += static_cast<char>(0x80 | (x & 0x3F));
        }
    }
    return out;
}

vector<u32string> words(const string& s) {
    vector<u32string> out;
    u32string word;
    for (char32_t c : decode(s)) {
        if (c >= U'A' && c <= U'Z') c += U'a' - U'A';
        if (c >= U'А' && c <= U'Я') c += U'а' - U'А';
        if (c == U'Ё') c = U'ё';
        bool letter = (c >= U'a' && c <= U'z') ||
                      (c >= U'а' && c <= U'я') || c == U'ё';
        if (letter) word += c;
        else if (!word.empty()) {
            out.push_back(word);
            word.clear();
        }
    }
    if (!word.empty()) out.push_back(word);
    return out;
}

Table::Table() : a(16) {}

uint64_t hash_word(const u32string& s) {
    uint64_t h = 0;
    for (char32_t c : s) h = h * 131 + static_cast<uint64_t>(c);
    return h;
}

const Word* find(const Table& t, const u32string& s) {
    size_t k = hash_word(s) % t.a.size();
    for (const Word& w : t.a[k]) {
        if (w.text == s) return &w;
    }
    return nullptr;
}

void rehash(Table& t, size_t m) {
    if (m == 0) m = 1;
    vector<list<Word>> b(m);
    for (auto& bucket : t.a) {
        while (!bucket.empty()) {
            auto p = bucket.begin();
            size_t k = hash_word(p->text) % m;

            b[k].splice(b[k].end(), bucket, p);
        }
    }
    t.a.swap(b);
}

void add(Table& t, const u32string& s, Hit hit) {
    size_t k = hash_word(s) % t.a.size();
    for (Word& w : t.a[k]) {
        if (w.text == s) {
            w.hits.push_back(hit);
            return;
        }
    }
    if (t.n + 1 > 0.75 * t.a.size()) rehash(t, 2 * t.a.size());
    k = hash_word(s) % t.a.size();
    t.a[k].push_back(Word{s, {hit}});
    ++t.n;
}

size_t edit_distance(const u32string& a, const u32string& b) {
    vector<size_t> prev(b.size() + 1), cur(b.size() + 1);
    for (size_t j = 0; j <= b.size(); ++j) prev[j] = j;
    for (size_t i = 1; i <= a.size(); ++i) {
        cur[0] = i;
        for (size_t j = 1; j <= b.size(); ++j) {
            size_t cost = a[i - 1] == b[j - 1] ? 0 : 1;
            cur[j] = min({prev[j] + 1, cur[j - 1] + 1, prev[j - 1] + cost});
        }
        prev.swap(cur);
    }
    return prev[b.size()];
}

vector<Match> similar(const Table& t, const u32string& s, size_t limit) {
    vector<Match> out;
    for (const auto& bucket : t.a) {
        for (const Word& w : bucket) {
            size_t a = s.size(), b = w.text.size();
            size_t diff = a > b ? a - b : b - a;
            if (diff > limit) continue;
            size_t d = edit_distance(s, w.text);
            if (d <= limit) out.push_back(Match{w.text, d});
        }
    }
    sort(out.begin(), out.end(), [](const Match& a, const Match& b) {
        if (a.dist != b.dist) return a.dist < b.dist;
        return a.word < b.word;
    });
    return out;
}
