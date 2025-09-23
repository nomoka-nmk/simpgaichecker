#include <bits/stdc++.h>
#include <filesystem>

namespace fs = std::filesystem;
using namespace std;

/*
General Config File Format
Time limit (ms)
Memory limit (kb)
*/

const fs::path output_logs = fs::current_path() / "LOGS" / "output_logs.txt";

string rtrim(const string& s) {

    const size_t end = s.find_last_not_of(" \t\r\n");
    return (end == string::npos) ? "" : s.substr(0, end + 1);

}

void write_To_output_logs(const string &submission, const string &testcase, const string& content) {

    // "wtf" means write to file
    ofstream wtf(output_logs.string(), ios::app);

    wtf << submission << " - " << testcase << ": " << content << '\n';

    wtf.close();

}

string compare_Files(const fs::path& output, const fs::path& answer) {

    if (!fs::exists(output)) {

        return "Error: Output file not found: " + output.string();

    }
    if (!fs::exists(answer)) {

        return "Error: Answer file not found: " + answer.string();

    }

    try {

        ifstream inf(output);
        ifstream ans(answer);

        if (!inf.is_open()) {

            return "Error: Cannot open output file";

        }
        if (!ans.is_open()) {

            return "Error: Cannot open answer file";

        }

        string expected, actual;
        int lineNum = 1;

        while (getline(ans, expected)) {

            if (!getline(inf, actual)) {

                return "WA: Output is shorter than expected at line " + to_string(lineNum);

            }

            if (expected != actual) {

                return "WA: Difference at line " + to_string(lineNum) + " | Expected: [" + expected + "]" + " | Actual: [" + actual + "]";

            }

            lineNum++;

        }

        if (getline(inf, actual)) {

            return "WA: Output is longer than expected. Extra content at line " + to_string(lineNum);

        }

        return "AC: Answer accepted";

    } catch (const exception& e) {

        return "Error: " + string(e.what());

    }

}

void scan_Submissions(vector<pair<int, string>> &submissions, const fs::path& SUBMISSION_DIR){

    for (const auto& entry : fs::directory_iterator(SUBMISSION_DIR)) {

        if (fs::is_regular_file(entry.path())) {

            submissions.emplace_back(submissions.size(), entry.path().filename().string());

        }

    }

}

void scan_Testcases(vector<pair<int, string>> &testcases, const fs::path& TESTCASES_DIR){

    for (const auto& entry : fs::directory_iterator(TESTCASES_DIR)) {

        if (fs::is_directory(entry.path())) {

            testcases.emplace_back(testcases.size(), entry.path().filename().string());

        }

    }

}

void build_Judge_Queue(queue<pair<int, int>> &judge_Queue, const vector<pair<int, string>>& submissions, const vector<pair<int, string>>& testcases){

    vector<pair<int, string>> standardized_Submissions;
    vector<pair<int, string>> standardized_Testcases;

    for (const auto&[fst, snd] : submissions) {

        string s = snd;

        ranges::transform(s, s.begin(),[](const unsigned char c){ return std::tolower(c); });

        string ext = ".cpp";

        if (const size_t pos = s.find(ext); pos != string::npos) {

            s.erase(pos, ext.size());

        }

        standardized_Submissions.emplace_back(fst, s);

    }

    for (const auto&[fst, snd] : testcases) {

        string s = snd;

        ranges::transform(s, s.begin(),[](const unsigned char c){ return std::tolower(c); });

        standardized_Testcases.emplace_back(fst, s);

    }

    for (const auto&[sfst, ssnd] : standardized_Submissions) {

        for (const auto&[tfst, tsnd] : standardized_Testcases) {

            if (ssnd == tsnd) {

                judge_Queue.emplace(sfst, tfst);

            }

        }

    }

}

int build_Execute(const fs::path& WORK_FILE, const fs::path& EXECUTE_FILE) {

    const string build_Flag = "-std=c++14 -pipe -O2 -s -static -lm -x c++ -Wl,--stack,66060288";
    const string file_input = '"' + WORK_FILE.string() + '"';
    const string file_output = '"' + EXECUTE_FILE.string() + '"';
    const string command = "g++ " + build_Flag + " " + file_input + " -o " + file_output;

    return system(command.c_str());

}

void generate_Config (const fs::path& CONFIGS_DIR, vector<pair<int, string>> &testcases) {

    const fs::path DEFAULT_CONFIG = CONFIGS_DIR / "default_config.txt";

    if (!fs::exists(DEFAULT_CONFIG)) {

        ofstream default_config(DEFAULT_CONFIG);
        default_config << "1000\n262144";

    }

    for (const auto &val: testcases | views::values) {

        string testcase = val;

        if (fs::path TESTCASE_CONFIG = CONFIGS_DIR / (testcase + ".txt"); !fs::exists(TESTCASE_CONFIG)) {

            fs::copy(DEFAULT_CONFIG, TESTCASE_CONFIG);

        }

    }

}

void get_Config(const fs::path& CONFIGS_DIR, const string& testcase, int &time_limit, int &memory_limit) {

    const fs::path TESTCASE_CONFIG = CONFIGS_DIR / (testcase + ".txt");
    ifstream testcase_config(TESTCASE_CONFIG.string());

    testcase_config >> time_limit >> memory_limit;

}

void Judge(const fs::path& SUBMISSION_DIR, const fs::path& TESTCASES_DIR, const fs::path& WORK_DIR, const fs::path& CONFIGS_DIR, const string& submission, const string& testcase) {

    const fs::path WORK_FILE = WORK_DIR / "main.cpp";
    const fs::path EXECUTE_FILE = WORK_DIR / "main.exe";

    if (fs::exists(WORK_DIR)) {

        fs::remove_all(WORK_DIR);

    }

    fs::create_directory(WORK_DIR);
    fs::copy_file((SUBMISSION_DIR / submission), WORK_FILE);

    if (build_Execute(WORK_FILE, EXECUTE_FILE) != 0) {

        write_To_output_logs(submission, testcase, "Error: Build failed");

        return;

    }

    int time_limit, memory_limit;

    get_Config(WORK_DIR, submission, time_limit, memory_limit);

}

void run_App() {

    if (fs::exists(output_logs)) fs::remove_all(output_logs);

    const fs::path SUBMISSION_DIR = fs::current_path() / "SUBMISSION";
    const fs::path TESTCASES_DIR = fs::current_path() / "TESTCASES";
    const fs::path CONFIGS_DIR = fs::current_path() / "CONFIGS";
    const fs::path WORK_DIR = fs::current_path() / "WORK";
    const fs::path LOGS_DIR = fs::current_path() / "LOGS";

    if (!fs::exists(SUBMISSION_DIR)) {

        fs::create_directory(SUBMISSION_DIR);

    }

    if (!fs::exists(TESTCASES_DIR)) {

        fs::create_directory(TESTCASES_DIR);

    }

    if (!fs::exists(CONFIGS_DIR)) {

        fs::create_directory(CONFIGS_DIR);

    }

    if (!fs::exists(LOGS_DIR)) {

        fs::create_directory(LOGS_DIR);

    }

    vector<pair<int, string>> submissions;
    vector<pair<int, string>> testcases;

    scan_Submissions(submissions, SUBMISSION_DIR);
    scan_Testcases(testcases, TESTCASES_DIR);

    generate_Config(CONFIGS_DIR, testcases);

    queue<pair<int, int>> judge_Queue;

    build_Judge_Queue(judge_Queue, submissions, testcases);

    while (!judge_Queue.empty()) {

        string submission = submissions[judge_Queue.front().first].second;
        string testcase = testcases[judge_Queue.front().second].second;
        judge_Queue.pop();

        Judge(SUBMISSION_DIR, TESTCASES_DIR, WORK_DIR, CONFIGS_DIR, submission, testcase);

    }

}

int main() {

    run_App();

    return 0;

}
