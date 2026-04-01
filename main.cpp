#include <bits/stdc++.h>
// We will implement a minimal NFA-based regex for alphabet {a,b} with ops +,*,?,| and concatenation
// No parentheses; '|' has lowest precedence.

namespace Grammar {
enum class TransitionType { Epsilon, a, b };
struct Transition { TransitionType type; int to; Transition(TransitionType t,int v):type(t),to(v){} };

class NFA {
  int start;
  std::unordered_set<int> ends;
  std::vector<std::vector<Transition>> transitions;
public:
  NFA() = default;
  ~NFA() = default;
  static NFA Epsilon(){
    NFA e; e.start=0; e.ends.clear(); e.ends.insert(0); e.transitions.assign(1, {}); return e;
  }

  std::unordered_set<int> GetEpsilonClosure(std::unordered_set<int> states) const {
    std::unordered_set<int> closure;
    std::queue<int> q;
    for (auto s: states) if (!closure.count(s)) { closure.insert(s); q.push(s);} 
    while(!q.empty()){
      int u=q.front(); q.pop();
      for (auto const &tr: transitions[u]){
        if (tr.type==TransitionType::Epsilon && !closure.count(tr.to)){
          closure.insert(tr.to); q.push(tr.to);
        }
      }
    }
    return closure;
  }

  std::unordered_set<int> Advance(std::unordered_set<int> current_states, char character) const {
    auto cclosure = GetEpsilonClosure(current_states);
    std::unordered_set<int> next;
    for (int s: cclosure){
      for (auto const &tr: transitions[s]){
        if ((character=='a' && tr.type==TransitionType::a) || (character=='b' && tr.type==TransitionType::b)){
          next.insert(tr.to);
        }
      }
    }
    return GetEpsilonClosure(next);
  }

  bool IsAccepted(int state) const { return ends.find(state)!=ends.end(); }
  int GetStart() const { return start; }

  friend NFA MakeStar(const char &character);
  friend NFA MakePlus(const char &character);
  friend NFA MakeQuestion(const char &character);
  friend NFA MakeSimple(const char &character);
  friend NFA Concatenate(const NFA &nfa1, const NFA &nfa2);
  friend NFA Union(const NFA &nfa1, const NFA &nfa2);
};

static void ensure_size(std::vector<std::vector<Transition>> &tr, int n){
  if ((int)tr.size()<n) tr.resize(n);
}

NFA MakeStar(const char &character) {
  NFA nfa; nfa.start=0; nfa.ends.clear(); nfa.ends.insert(0);
  nfa.transitions.assign(1, {});
  if (character=='a') nfa.transitions[0].push_back({TransitionType::a,0});
  else nfa.transitions[0].push_back({TransitionType::b,0});
  return nfa;
}

NFA MakePlus(const char &character) {
  // one or more: loop like star but not accepting empty. Use two states 0(start) and 1(accept), with consuming edge 0->1 and 1->1
  NFA nfa; nfa.start=0; nfa.ends.clear(); nfa.ends.insert(1);
  nfa.transitions.assign(2, {});
  if (character=='a'){
    nfa.transitions[0].push_back({TransitionType::a,1});
    nfa.transitions[1].push_back({TransitionType::a,1});
  }else{
    nfa.transitions[0].push_back({TransitionType::b,1});
    nfa.transitions[1].push_back({TransitionType::b,1});
  }
  return nfa;
}

NFA MakeQuestion(const char &character) {
  // zero or one: either epsilon accept or single consuming edge to accept
  NFA nfa; nfa.start=0; nfa.ends.clear(); nfa.ends.insert(1); nfa.ends.insert(0);
  nfa.transitions.assign(2, {});
  if (character=='a') nfa.transitions[0].push_back({TransitionType::a,1});
  else nfa.transitions[0].push_back({TransitionType::b,1});
  // epsilon edges encoded by TransitionType::Epsilon
  nfa.transitions[0].push_back({TransitionType::Epsilon,1});
  return nfa;
}

NFA MakeSimple(const char &character) {
  NFA nfa; nfa.start=0; nfa.ends.clear(); nfa.ends.insert(1);
  nfa.transitions.assign(2, {});
  if (character=='a') nfa.transitions[0].push_back({TransitionType::a,1});
  else nfa.transitions[0].push_back({TransitionType::b,1});
  return nfa;
}

NFA Concatenate(const NFA &nfa1, const NFA &nfa2) {
  NFA res;
  int n1 = (int)nfa1.transitions.size();
  int n2 = (int)nfa2.transitions.size();
  res.start = nfa1.start;
  res.transitions = nfa1.transitions;
  // append nfa2 transitions with offset
  int offset = n1;
  ensure_size(res.transitions, n1 + n2);
  for (int i=0;i<n2;i++){
    std::vector<Transition> row;
    for (auto const &tr: nfa2.transitions[i]){
      row.push_back({tr.type, tr.to + offset});
    }
    res.transitions[i+offset] = std::move(row);
  }
  // connect ends of nfa1 via epsilon to start of nfa2
  for (int e : nfa1.ends){
    res.transitions[e].push_back({TransitionType::Epsilon, nfa2.start + offset});
  }
  // new ends are ends of nfa2 offset
  res.ends.clear();
  for (int e: nfa2.ends) res.ends.insert(e + offset);
  return res;
}

NFA Union(const NFA &nfa1, const NFA &nfa2) {
  NFA res;
  int n1 = (int)nfa1.transitions.size();
  int n2 = (int)nfa2.transitions.size();
  // new start 0, then copy nfa1 and nfa2 with offsets
  res.start = 0;
  res.transitions.assign(1 + n1 + n2, {});
  // copy nfa1
  for (int i=0;i<n1;i++){
    for (auto const &tr: nfa1.transitions[i]){
      res.transitions[1+i].push_back({tr.type, 1 + tr.to});
    }
  }
  // copy nfa2
  for (int i=0;i<n2;i++){
    for (auto const &tr: nfa2.transitions[i]){
      res.transitions[1+n1+i].push_back({tr.type, 1 + n1 + tr.to});
    }
  }
  // epsilon from new start to old starts
  res.transitions[0].push_back({TransitionType::Epsilon, 1 + nfa1.start});
  res.transitions[0].push_back({TransitionType::Epsilon, 1 + n1 + nfa2.start});
  // ends are union of both adjusted
  res.ends.clear();
  for (int e: nfa1.ends) res.ends.insert(1 + e);
  for (int e: nfa2.ends) res.ends.insert(1 + n1 + e);
  return res;
}

class RegexChecker {
  NFA nfa;
public:
  bool Check(const std::string &str) const {
    std::unordered_set<int> states = { nfa.GetStart() };
    states = nfa.GetEpsilonClosure(states);
    for (char c: str){
      if (c!='a' && c!='b') return false;
      states = nfa.Advance(states, c);
      if (states.empty()) return false;
    }
    for (int s: states) if (nfa.IsAccepted(s)) return true;
    return false;
  }

  static bool isAtom(char c){ return c=='a' || c=='b'; }

  RegexChecker(const std::string &regex){
    // Split by top-level '|'
    std::vector<std::string> alts;
    int depth=0; size_t last=0;
    for (size_t i=0;i<regex.size();++i){
      if (regex[i]=='|'){
        alts.push_back(regex.substr(last, i-last));
        last=i+1;
      }
    }
    alts.push_back(regex.substr(last));

    auto buildSeq = [&](const std::string &seq){
      bool has = false; NFA cur; // unused init
      for (size_t i=0;i<seq.size();){
        if (!isAtom(seq[i])) { i++; continue; }
        char atom = seq[i++];
        NFA piece = MakeSimple(atom);
        if (i<seq.size()){
          char op = seq[i];
          if (op=='*'){ piece = MakeStar(atom); i++; }
          else if (op=='+'){ piece = MakePlus(atom); i++; }
          else if (op=='?'){ piece = MakeQuestion(atom); i++; }
        }
        if (!has){ cur = piece; has=true; }
        else cur = Concatenate(cur, piece);
      }
      if (!has){ return NFA::Epsilon(); }
      return cur;
    };

    NFA built = buildSeq(alts[0]);
    for (size_t i=1;i<alts.size();++i){
      NFA rhs = buildSeq(alts[i]);
      built = Union(built, rhs);
    }
    nfa = built;
  }
};
} // namespace Grammar

using namespace std;
using namespace Grammar;

int main(){
  ios::sync_with_stdio(false); cin.tie(nullptr);
  string pattern; if(!(cin>>pattern)) return 0;
  RegexChecker rc(pattern);
  int m=0; if(!(cin>>m)) return 0; vector<string> queries(m); for(int i=0;i<m;++i) cin>>queries[i];
  for (int i=0;i<m;++i){ cout << (rc.Check(queries[i])?1:0); if (i+1<m) cout << "\n"; }
  return 0;
}
