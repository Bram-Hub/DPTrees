#include <string>
#include <map>
#include "davis_putnam.h"

// Constructor from input text.
FullStatement::FullStatement(const std::string& raw_stat, std::map<std::string,Atomic*>& atomics) {
	root_ = new Statement(raw_stat, atomics, orig);
	atomics_ = &(root_->s_atomics);
}

// Copy constructor.
FullStatement::FullStatement(const FullStatement& fs) {
	//root_ = copy(fs.root_);
	root_ = fs.root_->copy();
	orig = fs.orig;
	atomics_ = &(root_->s_atomics);
}

// Returns true if atomic in Statement tree.
bool FullStatement::containsAtomic(const std::string& a) const {
	if(atomics_->find(a) != atomics_->end()) { return true; }
	return false;
}

// Main solving function: iterates through Statement objects, sets value and simplifies.
void FullStatement::evaluate(const std::string& curr_a) {
	Statement* s_ptr = root_;
	while(true) {
		// Find leaf node or lowest node containing current atomic.
		while(s_ptr->op_sym != ' ') {
			if(!s_ptr->left_->set_val && s_ptr->left_->containsAtomic(curr_a)) {
				s_ptr = s_ptr->left_;
			} else if(!s_ptr->right_->set_val && s_ptr->right_->containsAtomic(curr_a)) {
				s_ptr = s_ptr->right_;
			} else { break; }
		}
		if(s_ptr->op_sym == ' ') { // Set value of leaf node.
			s_ptr->val = s_ptr->s_atomics[curr_a]->getValue();
			s_ptr->set_val = true;
			if(s_ptr->negated) { s_ptr->val = !s_ptr->val; }
		} else {
			s_ptr = s_ptr->evaluate();
			s_ptr->s_atomics.erase(curr_a);
		} // Reaching the root means no further evalution can be completed.
		if(!s_ptr->parent_) { break; }
		s_ptr = s_ptr->parent_; // Iterate upward.
	}
	if(root_->set_val) { // Value of root, if set, is value of FullStatement.
		val = s_ptr->val;
		set_val = true;
	}
	rewrite();
}

// Use Statement tree to rewrite text statement after revisions/simplifications.
void FullStatement::rewrite() {
	if(set_val && !val) { // False value.
		orig = "[False]";
		return;
	} else if(set_val && val) { // True value.
		orig = "[True]";
		return;
	}
	orig = rewrite(root_);
	redundancy(orig);
}

// Convert original input statements into Conjunctive Normal Form.
void FullStatement::convertCNF() {
	convertCNF(root_);
	rewrite();
}

// Recursive helper function of rewrite().
std::string FullStatement::rewrite(Statement* s) const {
	std::string syntax;
	if(s->negated) { syntax += '!'; }
	if(s->op_sym == ' ') {
		syntax += s->s_atomics.begin()->first;
		return syntax;
	}
	syntax += '(' + rewrite(s->left_) + s->op_sym + rewrite(s->right_) + ')';
	return syntax;
}

// Recursive helper function fo convertCNF().
void FullStatement::convertCNF(Statement* s) {
	if(s->op_sym == ' ') { return; }
	if(s->op_sym == '$') { s->elimConditional(); }
	else if(s->op_sym == '%') {
		/* Biconditionals split into conjuncted conditionals, need new parent node,
		   special case if biconditional is root. */
		bool root = false;
		if(s == root_) { root = true; }
		s = s->elimBiconditional();
		if(root) { root_ = s; }
	}
	if(s->negated) { s->DeMorgan(); }
	convertCNF(s->left_);
	convertCNF(s->right_);
	// For DNF expression, one or both children may be conjunctions -> 3 cases.
	if(s->op_sym == '|' && s->left_->op_sym == '&') {
		if(s->right_->op_sym == '&') {
			s->DistribDisjunct(true);
			s->left_->DistribDisjunct(false);
			s->right_->DistribDisjunct(false);
		} else { s->DistribDisjunct(true); }
	} else if(s->op_sym == '|' && s->right_->op_sym == '&') { s->DistribDisjunct(false); }
}