// g++ -std=c++17 -O2 liveness.cpp && ./a.out
#include <bits/stdc++.h>
using namespace std;

struct Instr
{
    vector<string> use;   // variables read
    optional<string> def; // variable written (if any)
};

int main()
{
    // toy program
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

    // dump adjacency list
    for (auto &[v, adj] : graph)
    {
        cout << v << ':';
        for (const string &n : adj)
            cout << ' ' << n;
        cout << '\n';
    }
}
