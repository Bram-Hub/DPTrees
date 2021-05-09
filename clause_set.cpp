#include <algorithm>
#include <string>
#include <vector>
#include <list>
#include <map>
#include "davis_putnam.h"

// Constructor from input statements, includes CNF conversion.
ClauseSet::ClauseSet(std::list<FullStatement>& premises) {
	std::list<FullStatement>::iterator s_itr;
	for(s_itr = premises.begin(); s_itr != premises.end(); ++s_itr) {
		s_itr->convertCNF();
		Statement* s_ptr = s_itr->getRoot();
		Clause cla;
		// Iterate through leaf nodes, insert literals into clauses.
		while(s_ptr->left_) { s_ptr = s_ptr->left_; }
		cla[{s_ptr->s_atomics.begin()->first, !s_ptr->negated}] = s_ptr->s_atomics.begin()->second;
		Statement* end = s_itr->getRoot(); // Rightmost leaf node.
		while(end->right_) { end = end->right_; }
		while(s_ptr != end) {
			while(s_ptr->parent_ && s_ptr->parent_->right_ == s_ptr) { s_ptr = s_ptr->parent_; }
			s_ptr = s_ptr->parent_->right_;
			if(s_ptr->parent_->op_sym == '&') { // Make new clauses after encountering conjunctions.
				clauses.push_back(cla);
				cla.clear();
			}
			while(s_ptr->left_) { s_ptr = s_ptr->left_; }
			cla[{s_ptr->s_atomics.begin()->first, !s_ptr->negated}] = s_ptr->s_atomics.begin()->second;
		}
		clauses.push_back(cla);
		if(s_itr != premises.begin()) { // For each new premise, add new literals to single map.
			premises.begin()->getRoot()->mergeAtomics(s_itr->getRoot()->s_atomics);
		}
	}
	atomics_ = &(premises.begin()->getRoot()->s_atomics);
}

// Main solving function for clauses.
bool ClauseSet::evaluate(const Literal& prev, uint index) {
	// Adds current state to output encoding.
	write((prev.second ? "" : "!") + prev.first, index);
	std::pair<bool,bool> result = emptyClause();
	// Terminate with either open or closed branch if needed.
	if(result.first) { return result.second; }
	// Proceed with smallest sized clause, unit preference resolution if possible.
	iterator min_itr = getSmallest();
	Literal lit = min_itr->begin()->first;
	std::pair<bool,bool> unit_neg;
	if(min_itr->size() == 1) {
		unit_neg.first = true;
		if(lit.second) { unit_neg.second = true; }
		else { unit_neg.second = false; }
	}
	lit.second = true;
	Literal neg_lit = {lit.first, false};
	std::list<Clause> clauses_saved(clauses); // Copy for alternate recursive branches.
	bool true_branch, false_branch;
	// If current clause is unit literal, only make one branch.
	if(!unit_neg.first || unit_neg.second) {
		bool erase;
		for(iterator itr = clauses.begin(); itr != clauses.end(); erase ? itr : ++itr) {
			erase = false;
			Clause::iterator c_itr = itr->find(lit);
			if(c_itr != itr->end()) { // Delete entire clause if literal found.
				itr = clauses.erase(itr);
				erase = true;
			} else {
				// Only remove negated literal from clause if found.
				c_itr = itr->find(neg_lit);
				if(c_itr != itr->end()) { itr->erase(c_itr); }
			}
		}
		true_branch = evaluate(lit, 2*index+1);
	}
	else { true_branch = false; }
	// Same as above, but setting current literal to false.
	if(!unit_neg.first || !unit_neg.second) {
		bool erase;
		for(iterator itr = clauses.begin(); itr != clauses.end(); erase ? itr : ++itr) {
			erase = false;
			Clause::iterator c_itr = itr->find(neg_lit);
			if(c_itr != itr->end()) {
				itr = clauses.erase(itr);
				erase = true;
			} else {
				c_itr = itr->find(lit);
				if(c_itr != itr->end()) { itr->erase(c_itr); }
			}
		}
		false_branch = evaluate(neg_lit, 2*index+2);
	}
	else { false_branch = false; }
	clauses = clauses_saved;
	return true_branch || false_branch;
}

// Returns clause with least number of literals.
ClauseSet::iterator ClauseSet::getSmallest() {
	iterator min_itr;
	uint size = atomics_->size()+1;
	for(iterator itr = clauses.begin(); itr != clauses.end(); ++itr) {
		if(itr->size() < size) {
			min_itr = itr;
			size = itr->size();
		}
	}
	return min_itr;
}

// Returns if terminating condition is met and whether branch is open or closed.
std::pair<bool,bool> ClauseSet::emptyClause() const {
	if(clauses.empty()) { return {true,true}; }
	for(const_iterator itr = clauses.begin(); itr != clauses.end(); ++itr) {
		if(itr->empty()) { return {true,false}; }
	}
	return {false,false};
}

// Tautology Elimination: deletes clauses containing both a literal and its negation.
bool ClauseSet::elimTaut() {
	bool elim = false;
	bool erase;
	for(iterator itr = clauses.begin(); itr != clauses.end(); erase ? itr : ++itr) {
		erase = false;
		Clause::iterator c_itr;
		for(c_itr = itr->begin(); c_itr != itr->end(); ++c_itr) {
			if(itr->find(negate(c_itr->first)) != itr->end()) {
				itr = clauses.erase(itr);
				erase = true;
				elim = true;
				break;
			}
		}
	}
	return elim;
}

// Subsumption Elimination: deletes clauses subsumed by other clauses.
bool ClauseSet::elimSub() {
	bool elim = false;
	// Copy clauses and sort by size.
	std::list<Clause> copy(clauses);
	copy.sort(sortClause);
	// Remove smallest and compare to each clause.
	while(copy.size() > 1) {
		Clause smallest = copy.front();
		copy.pop_front();
		bool erase;
		for(iterator itr = clauses.begin(); itr != clauses.end(); erase ? itr : ++itr) {
			erase = false;
			if(smallest == *itr) { continue; }
			bool subsume = true;
			Clause::iterator c_itr;
			/* Compare with each clause until finding literal in smaller clause not
			   found in larger clause. */ 
			for(c_itr = smallest.begin(); c_itr != smallest.end(); ++c_itr) { //!=
				if(itr->find(c_itr->first) == itr->end()) {
					subsume = false;
					break;
				}
			}
			if(subsume) {
				itr = clauses.erase(itr);
				erase = true;
				elim = true;
			}
		}
	}
	return elim;
}

// Pure Literal Elimination: remove clause if it contains literal never or always negated.
bool ClauseSet::elimPure() {
	bool elim = false;
	std::vector<std::string> del_atomics;
	std::vector<Clause*> del_pure;
	std::map<std::string, Atomic*>::iterator a_itr;
	// Iterate through atomics, search for negated and non-negated literals.
	for(a_itr = atomics_->begin(); a_itr != atomics_->end(); ++a_itr) {
		std::vector<Clause*> pure;
		bool true_lit = false;
		bool false_lit = false;
		for(iterator itr = clauses.begin(); itr != clauses.end(); ++itr) {
			if(itr->find({a_itr->first, true}) != itr->end()) {
				true_lit = true;
				pure.push_back(&(*itr));
			} else if(itr->find({a_itr->first, false}) != itr->end()) {
				false_lit = true;
				pure.push_back(&(*itr));
			}
			// Cut short if both negated and non-negated found.
			if(true_lit && false_lit) { break; }
		}
		if((true_lit || false_lit) && !(true_lit && false_lit)) {
			for(uint i=0; i < pure.size(); ++i) {
				std::vector<Clause*>::iterator pure_itr;
				pure_itr = std::find(del_pure.begin(), del_pure.end(), pure[i]);
				if(pure_itr == del_pure.end()) { del_pure.push_back(pure[i]); }
				elim = true;
			}
			del_atomics.push_back(a_itr->first);
		// Remove atomic if not found in any clauses (not updated elsewhere).
		} else if(!true_lit && !false_lit) { del_atomics.push_back(a_itr->first); }
	}
	for(uint i=0; i < del_atomics.size(); ++i) {
		atomics_->erase(atomics_->find(del_atomics[i]));
	}
	for(uint i=0; i < del_pure.size(); ++i) {
		iterator del_itr = std::find(clauses.begin(), clauses.end(), *del_pure[i]);
		clauses.erase(del_itr);
	}
	return elim;
}

// Main function for writing output solving tree graphic encoding.
void ClauseSet::write(const std::string& curr_atom, uint index) {
	// Backfill with blank elements to maintain heap order.
	while(index+1 > output_tree.size()) { output_tree.push_back("# "); }
	output_tree[index] = "";
	if(index) { output_tree[index] += "-" + curr_atom; } // Mark new branch with literal.
	output_tree[index] += " #";
	if(clauses.empty()) {
		output_tree[index] += " [True]"; // Terminate with open branch.
		return;
	}
	for(iterator itr = clauses.begin(); itr != clauses.end(); ++itr) {
		output_tree[index] += " {";
		for(Clause::iterator c_itr = itr->begin(); c_itr != itr->end(); ++c_itr) {
			if(c_itr != itr->begin()) { output_tree[index] += ","; }
			output_tree[index] += (c_itr->first.second ? "" : "!") + c_itr->first.first;
		}
		output_tree[index] += "}";
	}
	// Attempt each elimination strategy, add to output if successful.
	std::string elim;
	if(!index && elimTaut()) { // Only need tautology elimination once.
		elim = " >TautElim";
		writeElim(elim);
		output_tree[index] += elim;
	}
	if(elimSub()) {
		elim = " >SubElim";
		writeElim(elim);
		output_tree[index] += elim;
	}
	// More pure clauses can be generated after each successful attempt.
	while(elimPure()) {
		elim = " >PureElim";
		writeElim(elim);
		output_tree[index] += elim;
	}
}

// Helper output function, adds elimination strategy steps.
void ClauseSet::writeElim(std::string& elim) const {
	if(clauses.empty()) {
		elim += " [True]"; // Terminate with open branch.
		return;
	}
	for(const_iterator itr = clauses.begin(); itr != clauses.end(); ++itr) {
		elim += " {";
		for(Clause::const_iterator c_itr = itr->begin(); c_itr != itr->end(); ++c_itr) {
			if(c_itr != itr->begin()) { elim += ","; }
			elim += (c_itr->first.second ? "" : "!") + c_itr->first.first;
		}
		elim += "}";
	}
}

// Returns negated literal.
Literal negate(const Literal& lit) {
	Literal n_lit = {lit.first, !lit.second};
	return n_lit;
}

// Sort-by-size comparison function.
bool sortClause(const Clause& c1, const Clause& c2) {
	if(c1.size() < c2.size()) { return true; }
	return false;
}