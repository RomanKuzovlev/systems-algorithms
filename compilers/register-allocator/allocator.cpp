// Minimal liveness + interference graph + register assignment
// Compile: g++ -std=c++17 -O2 liveness.cpp && ./a.out
#include <bits/stdc++.h>
using namespace std;

struct Instr
{
    vector<string> use;
    optional<string> def;
};

int main()
{
    vector<Instr> prog = {
        {{"a", "b"}, "t1"},  // t1 = a + b
        {{"t1", "c"}, "t2"}, // t2 = t1 * c
        {{"t2"}, nullopt}    // return t2
    };

    size_t n = prog.size();
    vector<set<string>> in(n), out(n);

    // fixed‑point liveness (backwards)
    bool changed = true;
    while (changed)
    {
        changed = false;
        for (int i = n - 1; i >= 0; --i)
        {
            set<string> newOut = (i + 1 < n) ? in[i + 1] : set<string>{};
            set<string> newIn(prog[i].use.begin(), prog[i].use.end());
            if (prog[i].def)
                newOut.erase(*prog[i].def);
            newIn.insert(newOut.begin(), newOut.end());
            if (newIn != in[i] || newOut != out[i])
            {
                in[i] = move(newIn);
                out[i] = move(newOut);
                changed = true;
            }
        }
    }

    // build interference graph: DEF x live‑out
    unordered_map<string, unordered_set<string>> graph;
    for (size_t i = 0; i < n; ++i)
    {
        if (!prog[i].def)
            continue;
        const string &d = *prog[i].def;
        graph[d];
        for (const string &v : out[i])
            if (v != d)
            {
                graph[d].insert(v);
                graph[v].insert(d);
            }
    }

    const int K = 2; // number of available registers
    unordered_map<string, int> reg;
    for (auto &[v, _] : graph)
    {
        vector<bool> used(K, false);
        for (const string &neighbor : graph[v])
        {
            if (reg.count(neighbor) && reg[neighbor] != -1 && reg[neighbor] < K)
            {
                used[reg[neighbor]] = true;
            }
        }
        int r = -1;
        for (int k = 0; k < K; ++k)
        {
            if (!used[k])
            {
                r = k;
                break;
            }
        }
        reg[v] = r; // -1 means spilled
    }

    cout << "\nRegister assignment (−1 = spilled):\n";
    for (auto &[v, r] : reg)
    {
        cout << v << " → ";
        if (r == -1)
            cout << "spill";
        else
            cout << "r" << r;
        cout << '\n';
    }
}
