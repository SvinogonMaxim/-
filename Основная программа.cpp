#include "Библиотека.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#ifdef _WIN32
#include <windows.h>
#endif

using namespace std;
namespace fs = std::filesystem;

struct File {
    string name;
    vector<string> lines;
};

vector<File> read_files(const fs::path& folder, Table& t) {
    vector<fs::path> paths;
    for (const auto& item : fs::directory_iterator(folder)) {
        if (!item.is_regular_file()) continue;
        string ext = item.path().extension().u8string();
        for (char& c : ext) if (c >= 'A' && c <= 'Z') c += 'a' - 'A';
        if (ext == ".txt") paths.push_back(item.path());
    }
    sort(paths.begin(), paths.end());
    vector<File> files;
    for (const auto& path : paths) {
        ifstream in(path);
        if (!in) throw runtime_error("Не удалось открыть: " + path.u8string());
        File f;
        f.name = path.filename().u8string();
        string line;
        while (getline(in, line)) {

            f.lines.push_back(line);
            try {
                for (const auto& word : words(line)) {
                    add(t, word, Hit{files.size(), f.lines.size()});
                }
            } catch (const exception& e) {
                throw runtime_error(f.name + ": " + e.what());
            }
        }
        if (in.bad()) throw runtime_error("Ошибка чтения: " + f.name);
        files.push_back(f);
    }
    return files;
}

void show(const Word& w, const vector<File>& files) {
    cout << "\nСлово: " << encode(w.text) << ". Вхождений: " << w.hits.size() << '\n';
    size_t shown = 0;
    size_t last_file = files.size(), last_line = 0;
    for (Hit h : w.hits) {

        if (h.file == last_file && h.line == last_line) continue;
        last_file = h.file;
        last_line = h.line;
        if (shown == 20) {
            cout << "Показаны первые 20 строк с совпадениями.\n";
            break;
        }
        const File& f = files[h.file];
        cout << f.name << ", строка " << h.line << ": " << f.lines[h.line - 1] << '\n';
        ++shown;
    }
}

int main(int argc, char* argv[]) {
    (void)argc;
#ifdef _WIN32
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);

    vector<wchar_t> path(32768);
    DWORD len = GetModuleFileNameW(nullptr, path.data(), static_cast<DWORD>(path.size()));
    if (len == 0 || len >= path.size()) return 1;
    fs::path folder = fs::path(wstring(path.data(), len)).parent_path() / fs::u8path("Тексты");
    (void)argv;
#else
    fs::path folder = fs::absolute(argv[0]).parent_path() / fs::u8path("Тексты");
#endif
    cout << "МИНИ-ПОИСКОВИК\nРусский и английский текст. Файлы .txt в UTF-8.\n";
    cout << "Папка по умолчанию: " << folder.u8string() << '\n';
    cout << "Нажмите Enter или введите путь к другой папке: ";
    string input;
    if (!getline(cin, input)) return 0;
    if (!input.empty()) folder = fs::u8path(input);

    Table t;
    vector<File> files;
    try {
        files = read_files(folder, t);
    } catch (const exception& e) {
        cerr << "Загрузка остановлена: " << e.what() << '\n';
        return 1;
    }
    if (files.empty()) {
        cout << "В этой папке нет файлов .txt. Добавьте тексты и запустите программу снова.\n";
        return 0;
    }
    cout << "Загружено файлов: " << files.size() << ". Различных слов: " << t.n << '\n';

    while (true) {
        cout << "\n1 - точный поиск\n2 - поиск с опечаткой (до двух изменений)\n"
                "3 - сведения о таблице\n0 - выход\nВаш выбор: ";
        string mode;
        if (!getline(cin, mode) || mode == "0") break;
        if (mode == "3") {
            size_t longest = 0;
            for (const auto& b : t.a) longest = max(longest, b.size());
            cout << "Слов: " << t.n << "; корзин: " << t.a.size()
                 << "; заполненность: " << static_cast<double>(t.n) / t.a.size()
                 << "; самая длинная цепочка: " << longest << '\n';
            continue;
        }
        if (mode != "1" && mode != "2") {
            cout << "Введите 0, 1, 2 или 3.\n";
            continue;
        }
        cout << "Введите одно слово: ";
        if (!getline(cin, input)) break;
        vector<u32string> query;
        try { query = words(input); }
        catch (const exception& e) { cout << e.what() << '\n'; continue; }
        if (query.size() != 1) {
            cout << "Нужно одно слово из русских или английских букв.\n";
            continue;
        }
        if (mode == "1") {
            const Word* w = find(t, query[0]);
            if (w) show(*w, files);
            else cout << "Слово не найдено. Можно попробовать режим 2.\n";
        } else {
            vector<Match> found = similar(t, query[0], 2);
            if (found.empty()) cout << "Подходящих слов нет.\n";
            for (size_t i = 0; i < found.size() && i < 10; ++i) {
                cout << "\nЧисло изменений: " << found[i].dist;
                show(*find(t, found[i].word), files);
            }
            if (found.size() > 10) cout << "Показаны первые 10 подходящих слов.\n";
        }
    }
    return 0;
}
