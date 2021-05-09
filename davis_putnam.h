#ifndef davis_putnam_h_
#define davis_putnam_h_

#include <string>
#include <vector>
#include <list>
#include <map>

typedef unsigned int uint; //Hopefully this fixes the compilation errors

// Objects for setting and keeping track of truth values for atomic statements.
class Atomic {
public:
	Atomic();
	Atomic(const std::string& n) : name(n) {}

	// Accessors
	bool getValue() const;
	int getQuantity() const { return quantity; }
	const std::string& getName() const { return name; }

	// Modifiers
	void setValue(bool v);
	void unsetValue() { set_val = false; }
	void resetQuantity() { quantity = 0; }
	void operator++() { ++quantity; } // Increases for every occurance of atomic.

private:
	// Representation
	std::string name;
	int quantity = 0; // Number of occurances in all input statements.
	bool val, set_val=false; // Truth value and whether truth value has been set.
};


class FullStatement;
class ClauseSet;

// Node objects for storing binary and negation operators as well as relevant literals.
class Statement {
public:
	friend class FullStatement;
	friend class ClauseSet;

private:
	Statement() {}
	Statement(const std::string& raw_stat, std::map<std::string,Atomic*>& atomics, std::string& orig);
	bool containsAtomic(const std::string& a) const;

	// Construction/destruction helper functions.
	Statement* copy() const;
	void destroy();

	// Helper functions for solving.
	void mergeAtomics(const std::map<std::string, Atomic*>& atoms2);
	Statement* evaluate();
	void reassign(Statement& s2);
	void simplify(Statement* keep);

	// Helper functions for CNF conversion.
	void elimConditional();
	Statement* elimBiconditional();
	void DeMorgan();
	void DistribDisjunct(bool nested_left);

	// Representation
	char op_sym = ' '; // Binary operator symbol, space is used for atomic statements.
	bool negated = false; // Presence of negation operator
	bool val, set_val = false; // Truth value and whether truth value has been set.
	Statement* parent_ = NULL;
	Statement* left_ = NULL;
	Statement* right_ = NULL;
	std::map<std::string, Atomic*> s_atomics; // All literals beneath operator.
};

/* Top-level object for holding contained Statement objects. Uses tree structure to
   represent logical statements with multiple binary operators. */
class FullStatement {
public:
	FullStatement(const std::string& raw_stat, std::map<std::string,Atomic*>& atomics);
	FullStatement(const FullStatement& fs);
	~FullStatement() { root_->destroy(); }
	
	// Accessors
	const std::string& getOrig() const { return orig; }
	Statement* getRoot() const { return root_; }
	bool containsAtomic(const std::string& a) const;
	bool getValSet() const { return set_val; }
	bool getVal() const { return val; }
	
	// Main solving functions
	void evaluate(const std::string& curr_a);
	void rewrite();
	
	void convertCNF();

private:
	std::string rewrite(Statement* s) const;
	void convertCNF(Statement* s);

	// Representation
	Statement* root_ = NULL;
	bool val, set_val = false; // Truth value and whether truth value has been set.
	std::string orig; // Written logical expression.
	std::map<std::string, Atomic*>* atomics_; // All literals used in full statment.
};

typedef std::pair<std::string, bool> Literal;
typedef std::map<Literal, Atomic*> Clause;

// Alternate method for storing and solving logical arguments using CNF and clause conversion.
class ClauseSet {
public:
	typedef std::list<Clause>::iterator iterator;
	typedef std::list<Clause>::const_iterator const_iterator;

	ClauseSet(std::list<FullStatement>& premises);
	bool evaluate(const Literal& lit, uint index);

	// Accessors
	iterator getSmallest();
	const std::vector<std::string>& getOutput() const { return output_tree; }
	std::pair<bool,bool> emptyClause() const;

private:
	// Shortcut modifiers
	bool elimTaut();
	bool elimSub();
	bool elimPure();

	// Output writing functions
	void write(const std::string& curr_atom, uint index);
	void writeElim(std::string& elim) const;

	// Representation
	std::list<Clause> clauses;
	std::map<std::string, Atomic*>* atomics_; // All literals used in clauses.
	std::vector<std::string> output_tree; // Text for tree graphic encoding.
};

void redundancy(std::string& stat);
Literal negate(const Literal& lit);
bool sortClause(const Clause& c1, const Clause& c2);

#endif