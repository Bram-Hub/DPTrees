#include <iostream>
#include <string>
#include <vector>
#include <list>
#include <map>
#include "davis_putnam.h"

// Updates quantities of atomic statements after each solving step.
void recount(std::map<std::string,Atomic*>& atomics, const std::list<FullStatement>& full_statements) {
	std::map<std::string,Atomic*>::iterator a_itr;
	std::list<FullStatement>::const_iterator s_itr;
	// Loops through all atomics with inner loop through all statements.
	for(a_itr = atomics.begin(); a_itr != atomics.end(); ++a_itr) {
		a_itr->second->resetQuantity();
		for(s_itr = full_statements.begin(); s_itr != full_statements.end(); ++s_itr) {
			// Iterates through each text statement to find matching atomic substrings.
			uint pos = 0;
			while(pos < s_itr->getOrig().size()) {
				pos = s_itr->getOrig().find(a_itr->first, pos);
				if(pos < s_itr->getOrig().size()) {
					++(*a_itr->second);
					++pos;
				}
			}
		}
	}
}

// Generates text at current node in output tree encoding after each solving step.
void write_output(const std::list<FullStatement>& full_statements, const std::string& curr_atom,
				  std::vector<std::string>& output_tree, uint index) {
	// Fills in blank elements as needed to maintain heap order in vector.
	while(index+1 > output_tree.size()) { output_tree.push_back("# "); }
	output_tree[index] = "-" + curr_atom; // Marking new branch with literal.
	output_tree[index] += " #";
	if(full_statements.empty()) { output_tree[index] += " [True]"; } // Open branch termination.
	std::list<FullStatement>::const_iterator itr;
	for(itr = full_statements.begin(); itr != full_statements.end(); ++itr) {
		output_tree[index] += " " + itr->getOrig();
	}
}

// Main solving function when keeping premises as original statements.
bool dpSolve(const std::list<FullStatement>& full_statements, std::map<std::string,Atomic*>& atomics,
			 std::vector<std::string>& output_tree, uint index, bool& solved) {
	// Choose next atomic to set value based on highest number of occurances.
	Atomic* curr_atom;
	if(atomics.size()) {
		curr_atom = atomics.begin()->second;
		std::map<std::string,Atomic*>::iterator itr;
		for(itr = ++atomics.begin(); itr != atomics.end(); ++itr) {
			if(curr_atom->getQuantity() < itr->second->getQuantity()) {
				curr_atom = itr->second;
			}
		}
	}
	// Remove current atomic, it will not be needed deeper in recursive steps.
	atomics.erase(curr_atom->getName());

	// Set current atomic's value to true, evaluate statements based on this assumption.
	bool true_branch = true;
	curr_atom->setValue(true);
	std::list<FullStatement> full_statements_copy; // Evaluate and recurse with statement copies.
	std::list<FullStatement>::const_iterator c_itr;
	for(c_itr = full_statements.begin(); c_itr != full_statements.end(); ++c_itr) {
		full_statements_copy.push_back(FullStatement(*c_itr));
	}
	std::list<FullStatement>::iterator itr;
	bool erase;
	for(itr=full_statements_copy.begin(); itr != full_statements_copy.end(); erase ? itr : ++itr) {
		erase = false;
		if(itr->containsAtomic(curr_atom->getName())) {
			itr->evaluate(curr_atom->getName());
			// Terminate with closed branch if statement fully evaluated to 'false'.
			if(itr->getValSet() && !itr->getVal()) { true_branch = false; }
			// Delete statements that are fully evaluated to 'true'.
			else if(itr->getValSet() && itr->getVal()) {
				itr = full_statements_copy.erase(itr);
				erase = true;
			}
		}
	}
	recount(atomics, full_statements_copy); // Recount atomics after evaluation.
	write_output(full_statements_copy, curr_atom->getName(), output_tree, index);
	// Only recurse if unused atomics, branch is not closed, and remaining statements.
	if(atomics.size() && true_branch && full_statements_copy.size()) {
		true_branch = dpSolve(full_statements_copy, atomics, output_tree, 2*index+1, solved);
	}
	if(full_statements_copy.empty() || solved) { // Terminate open branch, immediate return.
		solved = true;
		atomics[curr_atom->getName()] = curr_atom;
		return true;
	}

	// Set current atomic's value to false, evaluate statements based on this assumption.
	// Same methods as above for evaluation and writing output.
	++index;
	bool false_branch = true;
	curr_atom->setValue(false);
	full_statements_copy.clear();
	recount(atomics, full_statements);
	for(c_itr = full_statements.begin(); c_itr != full_statements.end(); ++c_itr) {
		full_statements_copy.push_back(FullStatement(*c_itr));
	}
	for(itr=full_statements_copy.begin(); itr != full_statements_copy.end(); erase ? itr : ++itr) {
		erase = false;
		if(itr->containsAtomic(curr_atom->getName())) {
			itr->evaluate(curr_atom->getName());
			if(itr->getValSet() && !itr->getVal()) { false_branch = false; }
			else if(itr->getValSet() && itr->getVal()) {
				itr = full_statements_copy.erase(itr);
				erase = true;
			}
		}
	}
	recount(atomics, full_statements_copy);
	write_output(full_statements_copy, "!"+curr_atom->getName(), output_tree, index);
	if(atomics.size() && false_branch && full_statements_copy.size()) {
		false_branch = dpSolve(full_statements_copy, atomics, output_tree, 2*index+1, solved);
	}
	if(full_statements_copy.empty() || solved) {
		solved = true;
		atomics[curr_atom->getName()] = curr_atom;
		return true;
	}

	// Reset current atomic so that it can be reused for different recursive branches.
	curr_atom->unsetValue();
	atomics[curr_atom->getName()] = curr_atom;
	recount(atomics, full_statements);
	return true_branch || false_branch;
}

// Converting heap vector into output string encoding solved tree graphic.
void printTree(const std::vector<std::string>& output_tree, bool cnf) {
	std::string output(output_tree[0]);
	uint branch_index = 1;
	int node;
	for(uint i=1; i < output_tree.size(); ++i) {
		// Create alternating lines of node statements and branches marked with literals.
		if(i == branch_index) {
			output += "\n";
			branch_index = (branch_index+1)*2 - 1;
			for(uint j=i; j < branch_index && j < output_tree.size(); ++j) {
				// Case of blank branches/nodes.
				if(output_tree[j] == "# ") { output += "- "; }
				else {
					node = output_tree[j].find('#');
					// Prints literal on branch line.
					output += output_tree[j].substr(0, node);
				}
			}
			output += "\n";
		}
		node = output_tree[i].find('#');
		output += output_tree[i].substr(node); // Prints statements on node line.
		if(output_tree[i] != "# ") { output += " "; }
	}
	output += "\nend"; // Terminating tag.
	std::cout << output << std::endl;
}

int main() {
	std::string in_stat;
	std::list<FullStatement> full_statements;
	std::map<std::string,Atomic*> atomics;
	// Used for checking input, accepts operator shortcuts, letters, and parentheses.
	std::string valid_chars("!&|$%()ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz");
	bool cnf = false;
	std::cin >> in_stat;
	while(in_stat != "0") { // Input termination tag.
		if(in_stat == "-cnf") { // Use -cnf tag to switch to solving with clauses.
			cnf = true;
			std::cin >> in_stat;
			continue;
		}
		if(in_stat.back() != ';') { // Premise termination tag.
			std::string in_stat_more;
			// Keep reading in input until tag is reached. Allows for optional whitespace.
			while(std::cin >> in_stat_more) {
				in_stat += in_stat_more;
				if(in_stat.back() == ';') { break; }
			}
		}
		// Premise termination tag missing, should not occur through GUI.
		if(in_stat.back() != ';') {
			std::cerr << "Error: Incomplete logic statement input." << std::endl;
			exit(1);
		}
		if(in_stat.size() == 1) { // Empty input line case.
			std::cerr << "Error: Blank statement entered." << std::endl;
			exit(1);
		}
		in_stat.pop_back(); // Remove ';' tag.
		int left_par=0;
		int right_par=0;
		for(uint i=0; i<in_stat.size(); ++i) {
			if(in_stat[i] == '~') { in_stat[i] = '!'; } // Allows either negation shortcut.
			// Check that all input uses valid characters.
			if(valid_chars.find(in_stat[i]) > valid_chars.size()) {
				std::cerr << "Error: Invalid characters in " << in_stat << std::endl;
				exit(1);
			}
			// No atomics or closing parenthesis preceding opening parentheis.
			if(in_stat[i]=='(') {
				if(i>0 && !(in_stat[i-1]=='&' || in_stat[i-1]=='|' || in_stat[i-1]=='$' ||
				  in_stat[i-1]=='%' || in_stat[i-1]=='(' || in_stat[i-1]=='!')) {
					std::cerr << "Error: Improper logic expression: " << in_stat << std::endl;
					exit(1);
				}
				++left_par; }
			// No atomics or opening parenthesis following closing parentheis.
			if(in_stat[i]==')') {
				if(i+1 < in_stat.size() && !(in_stat[i+1]=='&' || in_stat[i+1]=='|' ||
				  in_stat[i+1]=='$' || in_stat[i+1]=='%' || in_stat[i+1]==')')) {
					std::cerr << "Error: Improper logic expression: " << in_stat << std::endl;
					exit(1);
				} // Number of closing parentheses can never exceed opening parentheses.
				if(left_par <= right_par) {
					std::cerr << "Error: Mismatched parentheses in " << in_stat << std::endl;
					exit(1);
				}
				++right_par;
			}
		}
		if(left_par != right_par) {
			std::cerr << "Error: Mismatched parentheses in " << in_stat << std::endl;
			exit(1);
		}
		full_statements.push_back(FullStatement(in_stat, atomics));
		std::cin >> in_stat;
	}
	if(full_statements.empty()) {
		std::cerr << "Error: No statements have been entered." << std::endl;
		exit(1);
	}
	bool consistent; // Will be true if open terminal branch, false if all branches close.
	if(cnf) {
		// Convert statements into CNF and then clauses if requested.
		ClauseSet clause_set(full_statements);
		consistent = clause_set.evaluate({"",true}, 0);
		printTree(clause_set.getOutput(), cnf);
	} else { // Solving with original statements.
		// Load output string encoding with original statements as root.
		std::vector<std::string> output_tree(1, "#");
		std::list<FullStatement>::iterator c_itr;
		for(c_itr = full_statements.begin(); c_itr != full_statements.end(); ++c_itr) {
			output_tree[0] += " " + c_itr->getOrig();
		}
		bool solved = false; // Allows immediate return after terminating open branch.
		consistent = dpSolve(full_statements, atomics, output_tree, 1, solved);
		printTree(output_tree, cnf);
	}
	std::cout << consistent << std::endl;
	// Deallocate dynamic memory containing atomic objects.
	std::map<std::string,Atomic*>::iterator itr;
	for(itr = atomics.begin(); itr != atomics.end(); ++itr) { delete itr->second; }
	return 0;
}