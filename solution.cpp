#include <queue>
#include <string>
#include <unordered_set>
#include <vector>

// Implementations for the Grammar namespace declared in the harness src.hpp
namespace Grammar {

// Forward declarations to satisfy out-of-class definitions
class NFA;
NFA MakeStar(const char &character);
NFA MakePlus(const char &character);
NFA MakeQuestion(const char &character);
NFA Concatenate(const NFA &nfa1, const NFA &nfa2);
NFA Union(const NFA &nfa1, const NFA &nfa2);
NFA MakeSimple(const char &character);

enum class TransitionType { Epsilon, a, b };
struct Transition {
  TransitionType type;
  int to;
  Transition(TransitionType type, int to) : type(type), to(to) {}
};

class NFA {
private:
  int start;
  std::unordered_set<int> ends;
  std::vector<std::vector<Transition>> transitions;

public:
  NFA() = default;
  ~NFA() = default;

  std::unordered_set<int>
  GetEpsilonClosure(std::unordered_set<int> states) const;

  std::unordered_set<int> Advance(std::unordered_set<int> current_states,
                                  char character) const;

  bool IsAccepted(int state) const;
  int GetStart() const;

  friend NFA MakeStar(const char &character);
  friend NFA MakePlus(const char &character);
  friend NFA MakeQuestion(const char &character);
  friend NFA MakeSimple(const char &character);
  friend NFA Concatenate(const NFA &nfa1, const NFA &nfa2);
  friend NFA Union(const NFA &nfa1, const NFA &nfa2);
};

// Definitions that rely on members of NFA
std::unordered_set<int>
NFA::Advance(std::unordered_set<int> current_states, char character) const {
  // epsilon-closure of current states
  std::unordered_set<int> closure = GetEpsilonClosure(current_states);
  std::unordered_set<int> next;
  for (const auto &s : closure) {
    for (const auto &tr : transitions[s]) {
      if ((character == 'a' && tr.type == TransitionType::a) ||
          (character == 'b' && tr.type == TransitionType::b)) {
        next.insert(tr.to);
      }
    }
  }
  return GetEpsilonClosure(next);
}

bool NFA::IsAccepted(int state) const {
  return ends.find(state) != ends.end();
}

int NFA::GetStart() const { return start; }

// Helper to ensure vector size
static void ensure_size(std::vector<std::vector<Transition>> &tr, int n) {
  if (static_cast<int>(tr.size()) < n)
    tr.resize(n);
}

// Provided in harness according to README; keep consistent here for safety
NFA MakeStar(const char &character) {
  NFA nfa;
  nfa.start = 0;
  nfa.ends.clear();
  nfa.ends.insert(0);
  nfa.transitions.assign(1, std::vector<Transition>());
  if (character == 'a') {
    nfa.transitions[0].push_back({TransitionType::a, 0});
  } else {
    nfa.transitions[0].push_back({TransitionType::b, 0});
  }
  return nfa;
}

NFA MakePlus(const char &character) {
  NFA nfa;
  nfa.start = 0;
  nfa.ends.clear();
  nfa.ends.insert(1);
  nfa.transitions.assign(2, std::vector<Transition>());
  if (character == 'a') {
    nfa.transitions[0].push_back({TransitionType::a, 1});
    nfa.transitions[1].push_back({TransitionType::a, 1});
  } else {
    nfa.transitions[0].push_back({TransitionType::b, 1});
    nfa.transitions[1].push_back({TransitionType::b, 1});
  }
  return nfa;
}

NFA MakeQuestion(const char &character) {
  NFA nfa;
  nfa.start = 0;
  nfa.ends.clear();
  nfa.ends.insert(0);
  nfa.ends.insert(1);
  nfa.transitions.assign(2, std::vector<Transition>());
  if (character == 'a') {
    nfa.transitions[0].push_back({TransitionType::a, 1});
  } else {
    nfa.transitions[0].push_back({TransitionType::b, 1});
  }
  // allow epsilon (zero occurrence)
  nfa.transitions[0].push_back({TransitionType::Epsilon, 1});
  return nfa;
}

NFA MakeSimple(const char &character) {
  NFA nfa;
  nfa.start = 0;
  nfa.ends.clear();
  nfa.ends.insert(1);
  nfa.transitions.assign(2, std::vector<Transition>());
  if (character == 'a') {
    nfa.transitions[0].push_back({TransitionType::a, 1});
  } else {
    nfa.transitions[0].push_back({TransitionType::b, 1});
  }
  return nfa;
}

NFA Concatenate(const NFA &nfa1, const NFA &nfa2) {
  NFA res;
  int n1 = static_cast<int>(nfa1.transitions.size());
  int n2 = static_cast<int>(nfa2.transitions.size());
  res.start = nfa1.start;
  res.ends.clear();
  res.transitions = nfa1.transitions;

  int offset = n1;
  ensure_size(res.transitions, n1 + n2);
  for (int i = 0; i < n2; ++i) {
    std::vector<Transition> row;
    for (const auto &tr : nfa2.transitions[i]) {
      row.push_back({tr.type, tr.to + offset});
    }
    res.transitions[i + offset] = row;
  }

  for (int e : nfa1.ends) {
    res.transitions[e].push_back({TransitionType::Epsilon, nfa2.start + offset});
  }

  for (int e : nfa2.ends)
    res.ends.insert(e + offset);
  return res;
}

NFA Union(const NFA &nfa1, const NFA &nfa2) {
  NFA res;
  int n1 = static_cast<int>(nfa1.transitions.size());
  int n2 = static_cast<int>(nfa2.transitions.size());
  res.start = 0;
  res.ends.clear();
  res.transitions.assign(1 + n1 + n2, std::vector<Transition>());

  for (int i = 0; i < n1; ++i) {
    for (const auto &tr : nfa1.transitions[i]) {
      res.transitions[1 + i].push_back({tr.type, 1 + tr.to});
    }
  }
  for (int i = 0; i < n2; ++i) {
    for (const auto &tr : nfa2.transitions[i]) {
      res.transitions[1 + n1 + i].push_back({tr.type, 1 + n1 + tr.to});
    }
  }
  res.transitions[0].push_back({TransitionType::Epsilon, 1 + nfa1.start});
  res.transitions[0].push_back({TransitionType::Epsilon, 1 + n1 + nfa2.start});

  for (int e : nfa1.ends) res.ends.insert(1 + e);
  for (int e : nfa2.ends) res.ends.insert(1 + n1 + e);
  return res;
}

class RegexChecker {
private:
  NFA nfa;

public:
  bool Check(const std::string &str) const;
  RegexChecker(const std::string &regex);
};

bool RegexChecker::Check(const std::string &str) const {
  std::unordered_set<int> states;
  states.insert(nfa.GetStart());
  states = nfa.GetEpsilonClosure(states);
  for (char c : str) {
    if (c != 'a' && c != 'b') return false;
    states = nfa.Advance(states, c);
    if (states.empty()) return false;
  }
  for (int s : states) if (nfa.IsAccepted(s)) return true;
  return false;
}

static bool isAtom(char c) { return c == 'a' || c == 'b'; }

RegexChecker::RegexChecker(const std::string &regex) {
  // Split by top-level '|'
  std::vector<std::string> alts;
  std::size_t last = 0;
  for (std::size_t i = 0; i < regex.size(); ++i) {
    if (regex[i] == '|') {
      alts.push_back(regex.substr(last, i - last));
      last = i + 1;
    }
  }
  alts.push_back(regex.substr(last));

  // Build concatenation for a sequence without '|'
  struct Builder {
    static NFA seq(const std::string &s) {
      bool has = false;
      NFA cur; // uninitialized until first piece
      std::size_t i = 0;
      while (i < s.size()) {
        if (!isAtom(s[i])) { ++i; continue; }
        char atom = s[i++];
        NFA piece = MakeSimple(atom);
        if (i < s.size()) {
          char op = s[i];
          if (op == '*') { piece = MakeStar(atom); ++i; }
          else if (op == '+') { piece = MakePlus(atom); ++i; }
          else if (op == '?') { piece = MakeQuestion(atom); ++i; }
        }
        if (!has) { cur = piece; has = true; }
        else { cur = Concatenate(cur, piece); }
      }
      // If empty, construct an NFA that matches nothing (won't be tested per statement)
      if (!has) {
        // Create a dead NFA: one state, no accepting states
        NFA dead;
        dead.start = 0;
        dead.ends.clear();
        dead.transitions.assign(1, std::vector<Transition>());
        return dead;
      }
      return cur;
    }
  };

  NFA built = Builder::seq(alts[0]);
  for (std::size_t i = 1; i < alts.size(); ++i) {
    NFA rhs = Builder::seq(alts[i]);
    built = Union(built, rhs);
  }
  nfa = built;
}

} // namespace Grammar

