#include <iostream>
#include <string>
#include <map>
#include "davis_putnam.h"

// Access set truth value of atomic.
bool Atomic::getValue() const {
	if(!set_val) {
		std::cerr << "Error: No set truth value of " << name << std::endl;
		exit(1);
	}
	return val;
}

// Set atomic's value to true or false.
void Atomic::setValue(bool v) {
	val = v;
	set_val = true;
}

// Constructor for Statement objects. Also fills maps for atomic objects.
Statement::Statement(const std::string& raw_stat, std::map<std::string,Atomic*>& atomics, std::string& orig) {
	std::string raw = raw_stat;
	redundancy(raw); // Remove excess parentheses.
	if(raw.back() == '!') { // Negation operator cannot be last character.
		std::cerr << "Error: Improper placement of negate operator in " << raw << std::endl;
		exit(1);
	}
	// Locate outer parenthesis set to determine central operator.
	uint i = raw.rfind(')');
	if(i < raw.size() && i == raw.size()-1){
		--i;
		int open_par = 1;
		while(open_par) {
			if(raw[i] == ')') { ++open_par; }
			else if(raw[i] == '(') { --open_par; }
			--i;
		}
	} else { i = raw.size()-1; }
	uint op_pos = raw.find_last_of("&|$%", i);
	if(op_pos > raw.size()) {
		uint parenth1 = raw.find('(');
		// Case in which outer parenthesis set is negated.
		if(parenth1 < raw.size() && raw.substr(0, parenth1) == std::string(parenth1, '!')) {
			raw = raw.substr(parenth1);
			std::string orig2;
			// Recurse to read-in statement within parentheses, copy info back to this node.
			Statement temp = Statement(raw, atomics, orig2);
			reassign(temp);
			negated = bool(parenth1 % 2);
			if(temp.negated) { negated = !negated; } // In case of nested negations.
			if(op_sym != ' ') { orig2 = '(' + orig2 + ')'; }
			orig = std::string(parenth1, '!') + orig2;
			return;
		}
		// Case of reaching literal.
		uint last_n = raw.find_first_not_of('!');
		negated = bool(last_n % 2);
		raw = raw.substr(last_n);
		if(raw.find('!') < raw.size()) {
			std::cerr << "Error: Improper placement of negate operator in " << raw << std::endl;
			exit(1);
		}
		// Create atomic object if first time encountered, add to map.
		std::map<std::string,Atomic*>::iterator itr = atomics.find(raw);
		if(itr == atomics.end()) {
			Atomic* new_atomic = new Atomic(raw);
			atomics.insert(std::make_pair(raw, new_atomic));
		}
		++(*atomics[raw]);
		s_atomics[raw] = atomics[raw];
		// Rewrite atomic statement with same number of negations (not preserved in solvers.)
		orig = std::string(last_n, '!') + raw;
		return;
	}
	// No binary operators as first/last character or adjacent to other operators and parentheses.
	if(op_pos==0 || op_pos==raw.size()-1 || raw[op_pos-1]=='(' || raw[op_pos+1]==')' || 
	  raw.find_first_of("&|$%", op_pos+1)==op_pos+1 || raw[op_pos-1]=='!') {
		std::cerr << "Error: Improper placement of operator in " << raw << std::endl;
		exit(1);
	}
	op_sym = raw[op_pos];
	std::string l_orig;
	std::string r_orig;
	// Recurse to make new statement from left and right sides of central operator.
	left_ = new Statement(raw.substr(0, op_pos), atomics, l_orig);
	left_->parent_ = this;
	right_ = new Statement(raw.substr(op_pos+1), atomics, r_orig);
	right_->parent_ = this;
	// Make atomics map containing union of left and right child nodes.
	s_atomics = left_->s_atomics;
	mergeAtomics(right_->s_atomics);
	// Rewrite text statement, adding parentheses to children if needed.
	if(left_->op_sym != ' ' && !left_->negated) {
		l_orig = '(' + l_orig + ')';
	}
	if(right_->op_sym != ' ' && !right_->negated) {
		r_orig = '(' + r_orig + ')';
	}
	orig = l_orig + op_sym + r_orig;
}

// Returns if Statement includes atomic.
bool Statement::containsAtomic(const std::string& a) const {
	if(s_atomics.find(a) != s_atomics.end()) { return true; }
	return false;
}

// Copy constructor helper function.
Statement* Statement::copy() const {
	Statement* copy_s = new Statement;
	copy_s->op_sym = op_sym;
	copy_s->negated = negated;
	copy_s->val = val;
	copy_s->set_val = set_val;
	copy_s->s_atomics = s_atomics;
	if(left_) {
		copy_s->left_ = left_->copy();
		copy_s->left_->parent_ = copy_s;
		copy_s->right_ = right_->copy();
		copy_s->right_->parent_ = copy_s;
	} else {
		copy_s->left_ = NULL;
		copy_s->right_ = NULL;
	}
	return copy_s;
}

// Destructor helper function.
void Statement::destroy() {
	if(left_) { left_->destroy(); }
	if(right_) { right_->destroy(); }
	delete this;
}

// Creates union of atomic elements from two maps.
void Statement::mergeAtomics(const std::map<std::string, Atomic*>& atoms2) {
	std::map<std::string, Atomic*>::const_iterator c_itr;
	for(c_itr = atoms2.begin(); c_itr != atoms2.end(); ++c_itr) {
		if(s_atomics.find(c_itr->first) == s_atomics.end()) {
			s_atomics[c_itr->first] = c_itr->second;
		}
	}
}

/* If left and/or right child has confirmed truth value, determines value of node or 
   simplifies statement.
   negated = true -> val = true -> false -> val = !negated
   negated = true -> val = false -> true -> val = negated
   negated = false -> val = true -> val = !negated
   negated = false -> val = false -> val = negated */
Statement* Statement::evaluate() {
	Statement* curr_s = this; // Fixes edge case where FullStatement root is simplified.
	if(left_->set_val && !right_->set_val) {
		if(op_sym == '&' && !left_->val) {
			val = negated;
			set_val = true;
		} else if(op_sym == '|' && left_->val) {
			val = !negated;
			set_val = true;
		} else if(op_sym == '$' && !left_->val) {
			val = !negated;
			set_val = true;
		} else {
			if(parent_) { curr_s = right_; }
			if(op_sym == '%' && !left_->val) { right_->negated = !right_->negated; }
			simplify(right_);
		}
	} else if(!left_->set_val && right_->set_val) {
		if(op_sym == '&' && !right_->val) {
			val = negated;
			set_val = true;
		} else if(right_->val && (op_sym == '|' || op_sym == '$')) {
			val = !negated;
			set_val = true;
		} else {
			if(parent_) { curr_s = left_; }
			// For conditional, negates antecedent if consequent is false.
			if(op_sym == '$' || (op_sym == '%' && !right_->val)) {
				left_->negated = !left_->negated;
			}
			simplify(left_);
		}
	} else if(left_->set_val && right_->set_val) {
		if(op_sym == '&') {
			val = (left_->val && right_->val);
			if(negated) { val = !val; }
			set_val = true;
		} else if(op_sym == '|') {
			val = (left_->val || right_->val);
			if(negated) { val = !val; }
			set_val = true;
		} else if(op_sym == '$') {
			val = (!left_->val || right_->val);
			if(negated) { val = !val; }
			set_val = true;
		} else if(op_sym == '%') {
			val = ((left_->val && right_->val) || (!left_->val && !right_->val));
			if(negated) { val = !val; }
			set_val = true;
		} else {
			std::cerr << "Error: Cannot evaluate undefined logical operator." << std::endl;
			exit(1);
		}
	} else { set_val=false; }
	return curr_s;
}

// Copies over information from paramater Statement, disconnects from attached nodes.
void Statement::reassign(Statement& s2) {
	op_sym = s2.op_sym;
	left_ = s2.left_;
	right_ = s2.right_;
	s2.left_ = NULL;
	s2.right_ = NULL;
	s_atomics = s2.s_atomics;
	if(left_) { left_->parent_ = this; }
	if(right_) { right_->parent_ = this; }
}

// After setting one child's value, parent node replaced by child without set value if appropriate.
void Statement::simplify(Statement* keep) {
	if(!parent_) { // If parent is root, child's info is copied over.
		if(keep == left_) { right_->destroy(); }
		else { left_->destroy(); }
		reassign(*keep);
		if(negated && keep->negated) { negated = false; }
		else if(negated || keep->negated) { negated = true; }
		else { negated = false; }
		delete keep;
		return;
	}
	// Normal case: reassign pointers to change structure of Statement tree.
	if(parent_->left_ == this) { parent_->left_ = keep; }
	else { parent_->right_ = keep; }
	if(keep == left_) { left_ = NULL; }
	else { right_ = NULL; }
	keep->parent_ = parent_;
	if(negated && keep->negated) { keep->negated = false; }
	else if(negated || keep->negated) { keep->negated = true; }
	else { keep->negated = false; }
	destroy();
}

// Replace conditional with 'or' operator, negate antecedent.
void Statement::elimConditional() {
	left_->negated = !left_->negated;
	op_sym = '|';
}

// Replace biconditional with conjunction of two disjunctions.
Statement* Statement::elimBiconditional() {
	// Make new parent node to join individual conditionals.
	Statement* new_parent = new Statement;
	// Transfer negation to new parent if needed.
	if(negated) {
		negated = false;
		new_parent->negated = true;
	}
	Statement* copy_s = copy();
	// Switch left and right components of copied node to get both conditional directions.
	Statement* temp = copy_s->left_;
	copy_s->left_ = copy_s->right_;
	copy_s->right_ = temp;
	elimConditional();
	copy_s->elimConditional();
	new_parent->op_sym = '&';
	new_parent->s_atomics = s_atomics;
	if(parent_) { // Connect new parent node to original parent node if needed.
		new_parent->parent_ = parent_;
		if(parent_->left_ == this) { parent_->left_ = new_parent; }
		else {parent_->right_ = new_parent; }
	}
	new_parent->left_ = this;
	parent_ = new_parent;
	new_parent->right_ = copy_s;
	copy_s->parent_ = new_parent;
	return new_parent; // Sets current node to new parent.
}

// Distributes negation and flips 'and' to 'or' or vice versa.
void Statement::DeMorgan() {
	negated = false;
	left_->negated = !left_->negated;
	right_->negated = !right_->negated;
	if(op_sym == '&') { op_sym = '|'; }
	else { op_sym = '&'; }
}

// Converts DNF substatement to CNF.
void Statement::DistribDisjunct(bool nested_left) {
	Statement* copy_s, *nested, *new_right = new Statement;
	new_right->op_sym = '|';
	new_right->parent_ = this;
	if(nested_left) { // Case of (A&B)|C.
		copy_s = right_->copy();
		new_right->right_ = right_;
		right_->parent_ = new_right;
		nested = left_;
	} else { // Case of C|(A&B).
		copy_s = left_->copy();
		new_right->right_ = left_;
		left_->parent_ = new_right;
		nested = right_;
	}
	new_right->left_ = nested->right_;
	nested->right_->parent_ = new_right;
	nested->right_ = copy_s;
	copy_s->parent_ = nested;
	nested->op_sym = '|';
	// End result: (A|C)&(B|C)
	op_sym = '&';
	left_ = nested;
	right_ = new_right;
	// Fix atomics maps for nodes now in CNF.
	left_->s_atomics = left_->left_->s_atomics;
	left_->mergeAtomics(left_->right_->s_atomics);
	right_->s_atomics = right_->left_->s_atomics;
	right_->mergeAtomics(right_->right_->s_atomics);
}

// Deletes excess outer parentheses from text statement.
void redundancy(std::string& stat) {
	if(stat[0] != '(') { return; }
	uint i = 1;
	uint redund = 1;
	while(stat[i] == '(') {
		++redund;
		++i;
	}
	int open_par = 0; // Keeps track of inner parentheses.
	for(i=redund; redund && i < (stat.size()-redund); ++i) {
		if(stat[i] == ')') { 
			if(open_par) { --open_par; }
			else { --redund; } // Decreases if inner closing parenthesis matches outer opening.
		}
		else if(stat[i] == '(') { ++open_par; }
	}
	stat = stat.substr(redund, stat.size()-2*redund);
}