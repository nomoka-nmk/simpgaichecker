#include <bits/stdc++.h>
#include <chrono>
#include <filesystem>
#include <windows.h>
#include <psapi.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
namespace fs = std::filesystem;
using namespace std;

const fs::path output_logs = fs::current_path() / "LOGS" / "output_logs.json";

string rtrim(const string& s)
{

    const size_t end = s.find_last_not_of(" \t\r\n");
    return (end == string::npos) ? "" : s.substr(0, end + 1);

}

void write_To_output_logs(const string &submission, const string &testcase, const string& content,
                          const string& case_name, int time_used, int memory_used, const string& verdict)
{

    const json log_entry =
    {
        {"submission", submission},
        {"testcase", testcase},
        {"case", case_name},
        {"verdict", verdict},
        {"time_used", time_used},
        {"memory_used", memory_used},
        {"message", content},
        {"timestamp", chrono::system_clock::now().time_since_epoch().count()}
    };

    json full_log;

    ifstream in(output_logs.string());
    if (in)
    {
        try
        {
            in >> full_log;
        }
        catch (...)
        {
            full_log = json::object();
        }
    }
    in.close();

    if (!full_log.contains(testcase))
    {
        full_log[testcase] = json::array();
    }

    full_log[testcase].push_back(log_entry);

    ofstream out(output_logs.string());
    out << full_log.dump(4);
    out.close();
}

string compare_Files(const fs::path& output, const fs::path& answer)
{

    if (!fs::exists(output))
    {

        return "Error: Output file not found: " + output.string();

    }
    if (!fs::exists(answer))
    {

        return "Error: Answer file not found: " + answer.string();

    }

    try
    {

        ifstream inf(output);
        ifstream ans(answer);

        if (!inf.is_open())
        {

            return "Error: Cannot open output file";

        }
        if (!ans.is_open())
        {

            return "Error: Cannot open answer file";

        }

        string expected, actual;
        int lineNum = 1;

        while (getline(ans, expected))
        {

            if (!getline(inf, actual))
            {

                return "WA: Output is shorter than expected at line " + to_string(lineNum);

            }

            if (expected != actual)
            {

                return "WA: Difference at line " + to_string(lineNum) + " | Expected: [" + expected + "]" + " | Actual: [" += actual + "]";

            }

            lineNum++;

        }

        if (getline(inf, actual))
        {

            return "WA: Output is longer than expected. Extra content at line " + to_string(lineNum);

        }

        return "AC: Answer accepted";

    }
    catch (const exception& e)
    {

        return "Error: " + string(e.what());

    }

}

void scan_Submissions(vector<pair<int, string>> &submissions, const fs::path& SUBMISSION_DIR)
{

    for (const auto& entry : fs::directory_iterator(SUBMISSION_DIR))
    {

        if (fs::is_regular_file(entry.path()))
        {

            submissions.emplace_back(submissions.size(), entry.path().filename().string());

        }

    }

}

void scan_Testcases(vector<pair<int, string>> &testcases, const fs::path& TESTCASES_DIR)
{

    for (const auto& entry : fs::directory_iterator(TESTCASES_DIR))
    {

        if (fs::is_directory(entry.path()))
        {

            testcases.emplace_back(testcases.size(), entry.path().filename().string());

        }

    }

}

void build_Judge_Queue(queue<pair<int, int>> &judge_Queue, const vector<pair<int, string>>& submissions, const vector<pair<int, string>>& testcases)
{

    vector<pair<int, string>> standardized_Submissions;
    vector<pair<int, string>> standardized_Testcases;

    for (const auto&[fst, snd] : submissions)
    {

        string s = snd;

        ranges::transform(s, s.begin(),[](const unsigned char c)
        {
            return std::tolower(c);
        });

        string ext = ".cpp";

        if (const size_t pos = s.find(ext); pos != string::npos)
        {

            s.erase(pos, ext.size());

        }

        standardized_Submissions.emplace_back(fst, s);

    }

    for (const auto&[fst, snd] : testcases)
    {

        string s = snd;

        ranges::transform(s, s.begin(),[](const unsigned char c)
        {
            return std::tolower(c);
        });

        standardized_Testcases.emplace_back(fst, s);

    }

    for (const auto&[sfst, ssnd] : standardized_Submissions)
    {

        for (const auto&[tfst, tsnd] : standardized_Testcases)
        {

            if (ssnd == tsnd)
            {

                judge_Queue.emplace(sfst, tfst);

            }

        }

    }

}

int build_Execute(const fs::path& WORK_FILE, const fs::path& EXECUTE_FILE, const string& submission, const string& testcase)
{
    const string build_Flag = "-std=c++14 -pipe -O2 -s -static -lm -x c++ -Wl,--stack,66060288";
    const string file_input = '"' + WORK_FILE.string() + '"';
    const string file_output = '"' + EXECUTE_FILE.string() + '"';
    const string command = "g++ " + build_Flag + " " + file_input + " -o " + file_output + " 2> build_log.txt";

    int result = system(command.c_str());

    string build_log_content;
    ifstream log_file("build_log.txt");
    if (log_file.is_open())
    {
        string line;
        while (getline(log_file, line))
        {
            build_log_content += line + "\n";
        }
        log_file.close();
    }

    if (!build_log_content.empty() || result != 0)
    {
        write_To_output_logs(submission, testcase, build_log_content,
                             "build", 0, 0, (result == 0) ? "Build Success" : "CE");
    }

    fs::remove("build_log.txt");

    return result;
}

void generate_Config(const fs::path& CONFIGS_DIR, vector<pair<int, string>> &testcases)
{
    const fs::path DEFAULT_CONFIG = CONFIGS_DIR / "default_config.json";

    if (!fs::exists(DEFAULT_CONFIG))
    {
        json default_config =
        {
            {"time_limit", 1000},
            {"memory_limit", 262144},
            {"language", "cpp"}
        };

        ofstream config_file(DEFAULT_CONFIG);
        config_file << default_config.dump(4);
        config_file.close();
    }

    for (const auto &val: testcases | views::values)
    {
        fs::path TESTCASE_CONFIG = CONFIGS_DIR / (val + ".json");

        if (!fs::exists(TESTCASE_CONFIG))
        {
            fs::copy(DEFAULT_CONFIG, TESTCASE_CONFIG);
        }
    }

}

void get_Config(const fs::path& CONFIGS_DIR, const string& testcase, int &time_limit, int &memory_limit)
{
    const fs::path TESTCASE_CONFIG = CONFIGS_DIR / (testcase + ".json");

    if (!fs::exists(TESTCASE_CONFIG))
    {
        time_limit = 1000;
        memory_limit = 262144;
        return;
    }

    try
    {
        ifstream config_file(TESTCASE_CONFIG);
        json config = json::parse(config_file);

        time_limit = config.value("time_limit", 1000);
        memory_limit = config.value("memory_limit", 262144);

    }
    catch ([[maybe_unused]] const exception& e)
    {
        time_limit = 1000;
        memory_limit = 262144;
    }
}

vector<fs::path> get_testcase_dir (const fs::path& TESTCASES_DIR)
{

    vector<fs::path> testcase;

    for (const auto& entry : fs::directory_iterator(TESTCASES_DIR))
    {

        testcase.emplace_back(entry.path());

    }

    return testcase;

}

int execute_Submission(const fs::path& WORK_DIR, const fs::path& EXECUTE_FILE, int &time_used, const int time_limit, int &memory_used, const int memory_limit)
{
    STARTUPINFO si = { sizeof(si) };
    PROCESS_INFORMATION pi = {nullptr};
    std::wstring command = EXECUTE_FILE.wstring();
    const std::wstring work_dir = WORK_DIR.wstring();

    auto cleanup = [&pi]()
    {
        if (pi.hProcess)
        {
            TerminateProcess(pi.hProcess, 1);
            WaitForSingleObject(pi.hProcess, 5000);
            CloseHandle(pi.hProcess);
        }
        if (pi.hThread) CloseHandle(pi.hThread);
    };

    const auto start_time = std::chrono::high_resolution_clock::now();

    if (!CreateProcessW(
                nullptr,
                command.data(),
                nullptr,
                nullptr,
                FALSE,
                CREATE_NO_WINDOW | CREATE_NEW_PROCESS_GROUP,
                nullptr,
                work_dir.c_str(),
                reinterpret_cast<LPSTARTUPINFOW>(&si),
                &pi
            )) return -1;

    DWORD exitCode = STILL_ACTIVE;
    PROCESS_MEMORY_COUNTERS pmc;

    while (true)
    {
        const DWORD waitResult = WaitForSingleObject(pi.hProcess, 10);

        auto current_time = std::chrono::high_resolution_clock::now();
        const auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                                    current_time - start_time).count();

        time_used = static_cast<int>(elapsed_ms);

        if (time_used > time_limit)
        {
            cleanup();
            return 1;
        }

        if (waitResult == WAIT_OBJECT_0)
        {
            break;
        }
        else if (waitResult == WAIT_FAILED)
        {
            cleanup();
            return -1;
        }

        if (!GetProcessMemoryInfo(pi.hProcess, &pmc, sizeof(pmc)))
        {
            cleanup();
            return -1;
        }

        memory_used = static_cast<int>(pmc.PeakWorkingSetSize / 1024);
        if (memory_used > memory_limit)
        {
            cleanup();
            return 2;
        }
    }

    const auto end_time = std::chrono::high_resolution_clock::now();
    time_used = static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(
                                     end_time - start_time).count());

    GetExitCodeProcess(pi.hProcess, &exitCode);
    GetProcessMemoryInfo(pi.hProcess, &pmc, sizeof(pmc));
    memory_used = static_cast<int>(pmc.PeakWorkingSetSize / 1024);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return (exitCode == 0) ? 0 : -1;
}

void Judge(const fs::path& SUBMISSION_DIR, const fs::path& TESTCASES_DIR, const fs::path& WORK_DIR, const fs::path& CONFIGS_DIR, const string& submission, const string& testcase)
{

    const fs::path WORK_FILE = WORK_DIR / "main.cpp";
    const fs::path EXECUTE_FILE = WORK_DIR / "main.exe";

    if (fs::exists(WORK_DIR))
    {
        fs::remove_all(WORK_DIR);
    }

    fs::create_directory(WORK_DIR);
    fs::copy_file((SUBMISSION_DIR / submission), WORK_FILE);

    if (build_Execute(WORK_FILE, EXECUTE_FILE, submission, testcase) != 0)
    {

        return;
    }

    int time_limit, time_used, memory_limit, memory_used;
    get_Config(CONFIGS_DIR, testcase, time_limit, memory_limit);

    for (vector<fs::path> testcases = get_testcase_dir(TESTCASES_DIR / testcase); const auto& i : testcases)
    {

        fs::path INPUT_WORK_DIR = WORK_DIR / (testcase + ".inp");
        fs::path OUTPUT_WORK_DIR = WORK_DIR / (testcase + ".out");
        fs::path INPUT_TESTCASE_DIR = i / (testcase + ".inp");
        fs::path OUTPUT_TESTCASE_DIR = i / (testcase + ".out");

        fs::remove_all(INPUT_WORK_DIR);
        fs::remove_all(OUTPUT_WORK_DIR);
        fs::copy_file(INPUT_TESTCASE_DIR, INPUT_WORK_DIR);

        int execute_Result = execute_Submission(WORK_DIR, EXECUTE_FILE, time_used, time_limit, memory_used, memory_limit);

        string case_verdict = "AC";
        string message_content;

        if (execute_Result == 0)
        {
            string compare_result = compare_Files(OUTPUT_WORK_DIR, OUTPUT_TESTCASE_DIR);
            if (compare_result.find("AC:") != string::npos)
            {
                case_verdict = "AC";
                message_content = "Accepted";
            }
            else
            {
                case_verdict = "WA";
                message_content = compare_result;
            }
        }
        else if (execute_Result == 1)
        {
            case_verdict = "TLE";
            message_content = "Time Limit Exceeded";
        }
        else if (execute_Result == 2)
        {
            case_verdict = "MLE";
            message_content = "Memory Limit Exceeded";
        }
        else
        {
            case_verdict = "RTE";
            message_content = "Runtime Error";
        }

        write_To_output_logs(submission, testcase, message_content,
                             i.filename().string(), time_used, memory_used, case_verdict);
    }
}

string formatBuildLog(const string& raw_log)
{
    stringstream result;
    stringstream ss(raw_log);
    string line;

    while (getline(ss, line))
    {
        line = rtrim(line);
        if (line.empty()) continue;

        result << line << "\n";
    }

    return result.str();
}

void generate_Html()
{
    const fs::path json_file = fs::current_path() / "LOGS" / "output_logs.json";
    const fs::path html_file = fs::current_path() / "LOGS" / "results.html";

    if (!fs::exists(json_file)) return;

    ifstream inf(json_file.string());
    json data;
    inf >> data;
    inf.close();

    stringstream html;

    html << R"(
<!DOCTYPE html>
<html lang="vi">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Simp Gai Checker</title>
    <script src="https://cdn.tailwindcss.com"></script>
    <link rel="stylesheet" href="https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.4.0/css/all.min.css">
    <style>
        .verdict-ac { background-color: #dcfce7; color: #166534; }
        .verdict-wa { background-color: #fee2e2; color: #dc2626; }
        .verdict-tle { background-color: #fef3c7; color: #d97706; }
        .verdict-mle { background-color: #fce7f3; color: #be185d; }
        .verdict-rte { background-color: #f1f5f9; color: #475569; }
        .verdict-ce { background-color: #e0e7ff; color: #3730a3; }
        .progress-bar { transition: width 0.3s ease-in-out; }
    </style>
</head>
<body class="bg-gray-50 min-h-screen">
    <div class="container mx-auto px-4 py-8">
        <h1 class="text-3xl font-bold text-center text-gray-800 mb-2">
            <i class="fas fa-gavel mr-3"></i>Judge Results
        </h1>
        <p class="text-center text-gray-600 mb-8">Generated on: )" << __DATE__ << " " << __TIME__ << R"(</p>
    )";

    map<string, int> verdict_stats;
    int total_cases = 0;
    int total_time = 0;
    int max_memory = 0;

    for (const auto& [testcase, entries] : data.items())
    {
        for (const auto& entry : entries)
        {
            string verdict = entry["verdict"];
            verdict_stats[verdict]++;
            total_cases++;
            total_time += entry["time_used"].get<int>();
            max_memory = std::max(max_memory, entry["memory_used"].get<int>());
        }
    }

    html << R"(
        <div class="grid grid-cols-2 md:grid-cols-4 gap-4 mb-8">
            <div class="bg-white rounded-lg shadow p-4 text-center">
                <div class="text-2xl font-bold text-blue-600">)" << total_cases << R"(</div>
                <div class="text-gray-600">Total Cases</div>
            </div>
            <div class="bg-white rounded-lg shadow p-4 text-center">
                <div class="text-2xl font-bold text-green-600">)" << verdict_stats["AC"] << R"(</div>
                <div class="text-gray-600">Accepted</div>
            </div>
            <div class="bg-white rounded-lg shadow p-4 text-center">
                <div class="text-2xl font-bold text-red-600">)" << (total_cases - verdict_stats["AC"]) << R"(</div>
                <div class="text-gray-600">Failed</div>
            </div>
            <div class="bg-white rounded-lg shadow p-4 text-center">
                <div class="text-2xl font-bold text-purple-600">)" << (total_cases > 0 ? total_time / total_cases : 0) << R"(ms</div>
                <div class="text-gray-600">Avg Time</div>
            </div>
        </div>
    )";

    html << R"(
        <div class="bg-white rounded-lg shadow overflow-hidden">
            <div class="overflow-x-auto">
                <table class="min-w-full divide-y divide-gray-200">
                    <thead class="bg-gray-100">
                        <tr>
                            <th class="px-6 py-3 text-left text-xs font-medium text-gray-500 uppercase tracking-wider">Submission</th>
                            <th class="px-6 py-3 text-left text-xs font-medium text-gray-500 uppercase tracking-wider">Testcase</th>
                            <th class="px-6 py-3 text-left text-xs font-medium text-gray-500 uppercase tracking-wider">Case</th>
                            <th class="px-6 py-3 text-left text-xs font-medium text-gray-500 uppercase tracking-wider">Verdict</th>
                            <th class="px-6 py-3 text-left text-xs font-medium text-gray-500 uppercase tracking-wider">Time</th>
                            <th class="px-6 py-3 text-left text-xs font-medium text-gray-500 uppercase tracking-wider">Memory</th>
                            <th class="px-6 py-3 text-left text-xs font-medium text-gray-500 uppercase tracking-wider">Message</th>
                        </tr>
                    </thead>
                    <tbody class="bg-white divide-y divide-gray-200">
    )";

    for (const auto& [testcase, entries] : data.items())
    {
        for (const auto& entry : entries)
        {
            string verdict = entry["verdict"];
            string verdict_class = string("verdict-") +
                                   (verdict == "AC" ? "ac" :
                                    verdict == "WA" ? "wa" :
                                    verdict == "TLE" ? "tle" :
                                    verdict == "MLE" ? "mle" :
                                    verdict == "RTE" ? "rte" : "ce");

            string verdict_icon =
                verdict == "AC" ? "fa-check-circle" :
                verdict == "WA" ? "fa-times-circle" :
                verdict == "TLE" ? "fa-clock" :
                verdict == "MLE" ? "fa-memory" :
                verdict == "RTE" ? "fa-bug" : "fa-code";

            html << R"(
                        <tr class="hover:bg-gray-50">
                            <td class="px-6 py-4 whitespace-nowrap text-sm font-medium text-gray-900">)"
                 << entry["submission"] << R"(</td>
                            <td class="px-6 py-4 whitespace-nowrap text-sm text-gray-500">)"
                 << entry["testcase"] << R"(</td>
                            <td class="px-6 py-4 whitespace-nowrap text-sm text-gray-500">)"
                 << entry["case"] << R"(</td>
                            <td class="px-6 py-4 whitespace-nowrap">
                                <span class="px-2 py-1 text-xs font-semibold rounded-full )" << verdict_class << R"(">
                                    <i class="fas )" << verdict_icon << R"( mr-1"></i>)"
                 << verdict << R"(
                                </span>
                            </td>
                            <td class="px-6 py-4 whitespace-nowrap text-sm text-gray-500">)"
                 << entry["time_used"] << R"( ms</td>
                            <td class="px-6 py-4 whitespace-nowrap text-sm text-gray-500">)"
                 << entry["memory_used"] << R"( KB</td>
                            <td class="px-6 py-4 text-sm text-gray-500">)"
                 << entry["message"] << R"(</td>
                        </tr>
            )";
        }
    }

    html << R"(
                    </tbody>
                </table>
            </div>
        </div>

        <div class="mt-8 bg-white rounded-lg shadow overflow-hidden">
            <div class="px-6 py-4 bg-gray-100 border-b">
                <h2 class="text-lg font-semibold text-gray-800">
                    <i class="fas fa-hammer mr-2"></i>Build Logs
                </h2>
            </div>
            <div class="p-6">
    )";

    bool has_build_logs = false;
    for (const auto& [testcase, entries] : data.items())
    {
        for (const auto& entry : entries)
        {
            if (entry["case"] == "build")
            {
                has_build_logs = true;
                string verdict = entry["verdict"];
                string verdict_class = (verdict == "Build Success") ? "verdict-ac" : "verdict-ce";
                string verdict_text = (verdict == "Build Success") ? "Build Successful" : "Compilation Error";
                string message = entry["message"];

                html << R"(
                <div class="mb-6 border rounded-lg overflow-hidden">
                    <div class="px-4 py-3 )" << verdict_class << R"( border-b">
                        <strong>)" << entry["submission"] << R"(</strong> - )" << entry["testcase"] << R"(
                        <span class="float-right font-semibold">)" << verdict_text << R"(</span>
                    </div>
                    <div class="p-4 bg-gray-50">
                )";

                if (!message.empty())
                {
                    html << R"(<div class="build-log">)";

                    stringstream ss(message);
                    string line;
                    while (getline(ss, line))
                    {
                        line = rtrim(line);

                        if (line.empty()) continue;

                        if (line.find("error:") != string::npos)
                        {
                            html << R"(<div class="text-red-400 mb-1">)" << line << R"(</div>)";
                        }
                        else if (line.find("warning:") != string::npos)
                        {
                            html << R"(<div class="text-yellow-400 mb-1">)" << line << R"(</div>)";
                        }
                        else if (line.find("note:") != string::npos)
                        {
                            html << R"(<div class="text-blue-400 mb-1">)" << line << R"(</div>)";
                        }
                        else if (line.find(".cpp:") != string::npos)
                        {
                            html << R"(<div class="text-green-400 mb-1">)" << line << R"(</div>)";
                        }
                        else if (line.find('^') != string::npos)
                        {
                            html << R"(<div class="text-cyan-400 mb-1">)" << line << R"(</div>)";
                        }
                        else
                        {
                            html << R"(<div class="text-gray-300 mb-1">)" << line << R"(</div>)";
                        }
                    }

                    html << R"(</pre>)";
                }
                else
                {
                    html << R"(<p class="text-gray-500">No build messages</p>)";
                }

                html << R"(
                    </div>
                </div>
                )";
            }
        }
    }

    if (!has_build_logs)
    {
        html << R"(<p class="text-gray-500 text-center py-4">No build logs available</p>)";
    }

    html << R"(
            </div>
        </div>

        <div class="mt-8 text-center text-gray-500 text-sm">
            <i class="fas fa-code mr-1"></i> Generated by Simp Gai Checker
            <i class="fas fa-heart mx-2 text-red-500"></i>
            With love for you
        </div>
    </div>
</body>
</html>
    )";

    ofstream out(html_file.string());
    out << html.str();
    out.close();

    cout << "HTML report generated: " << html_file.string() << endl;
}

void run_App()
{

    if (fs::exists(output_logs)) fs::remove_all(output_logs);

    const fs::path SUBMISSION_DIR = fs::current_path() / "SUBMISSION";
    const fs::path TESTCASES_DIR = fs::current_path() / "TESTCASES";
    const fs::path CONFIGS_DIR = fs::current_path() / "CONFIGS";
    const fs::path WORK_DIR = fs::current_path() / "WORK";
    const fs::path LOGS_DIR = fs::current_path() / "LOGS";

    if (fs::exists(LOGS_DIR))
    {

        fs::remove_all(LOGS_DIR);

    }

    if (!fs::exists(SUBMISSION_DIR))
    {

        fs::create_directory(SUBMISSION_DIR);

    }

    if (!fs::exists(TESTCASES_DIR))
    {

        fs::create_directory(TESTCASES_DIR);

    }

    if (!fs::exists(CONFIGS_DIR))
    {

        fs::create_directory(CONFIGS_DIR);

    }

    if (!fs::exists(LOGS_DIR))
    {

        fs::create_directory(LOGS_DIR);

    }

    vector<pair<int, string>> submissions;
    vector<pair<int, string>> testcases;

    scan_Submissions(submissions, SUBMISSION_DIR);
    scan_Testcases(testcases, TESTCASES_DIR);

    generate_Config(CONFIGS_DIR, testcases);

    queue<pair<int, int>> judge_Queue;

    build_Judge_Queue(judge_Queue, submissions, testcases);

    while (!judge_Queue.empty())
    {

        string submission = submissions[judge_Queue.front().first].second;
        string testcase = testcases[judge_Queue.front().second].second;
        judge_Queue.pop();

        Judge(SUBMISSION_DIR, TESTCASES_DIR, WORK_DIR, CONFIGS_DIR, submission, testcase);

    }

    if (fs::exists(WORK_DIR)) fs::remove_all(WORK_DIR);

    generate_Html();

}

int main()
{

    run_App();

    return 0;

}
