/*
 * phc -- the open source PHP compiler
 * See doc/license/README.license for licensing information
 *
 * The lowering passes introduce a lot of unnecessary copies, which
 * add extra reference counts, resulting in extra copying and poor
 * performance.
 *
 * HACK TODO: Remove.
 * This is exactly the sort of hacky thing we wanted
 * to remove. This would be much better done with a data-flow
 * framework, but is much easier than cleaning up the passes, which
 * would otherwise be necessary.
 *
 * Since we do not have proper data-flow, we are going to be very
 * conservative, using the following assumptions:
 *
 *	-	Only do this for compiler generated temporaries, this is not
 *		for optimization of user-code, just as a clean-up.
 *
 * -	Simple example: :
 *		1.)	$L1 = $R1; // replaceable
 *		2.)	$L2 = $L1; // removable
 *		becomes:
 *		1.)	$L2 = $R1; // replaced
 *
 *	-	This is not the same as:
 *		1.)	$L1 = $R1;
 *		2.)	$L2 = $L1;	
 *		becoming:
 *		2.)	$L2 = $R1; // $R1 might have been assigned to in-between
 *			
 *	-	$L1 must be a simple variable, and must be annotated with
 *		phc.codegen.compiler_generated
 *
 *	-	Statement 1. must occur after statement 2.
 *
 *	-	Strange cases which break it, shouldn't happen because we only
 *		use this for the restricted set of compiler-generated
 *		temporaries.
 *
 *	The following restrictions may be temporary:
 *
 *	-	Both the assignments involved must be of the form
 *			$R = $L
 *		and must not be by reference, or involve array_indices.
 *	-	$L1 must be assigned exactly once.
 *	-	$L1 must be used exactly once - in the assignment to $L2.
 *	-	$L2 is assigned exactly once, by $R1
 *	-	We only work in methods, not in the global statement list
 */

#ifndef PHC_PROPAGATE_COPIES
#define PHC_PROPAGATE_COPIES

// TODO assignments by reference
// TODO function calls by reference

/*
foreach function as f:

	// count uses and defs
	pre_var { var_count [var] += 1; }
	pre_assignment { def_count [var] += 1}

	// make a use count from assignments
	foreach var_count, def_count:	
		use_count [var] = var_count [var] - def_count [var]

	// find potential statements
	stmts = f->stmts
	pre_stmt 
	{
		if s.is_simple_assignment

			l1 = s.lhs
			if (l1.is_compiler_var):
				assert replaceable [l1] == NULL
				replaceable [l1] = s

			// we can remove this if there is a matching replaceable
			// statement
			if replaceable [l1]
			&&	use_count [l1] == 1
			&&	def_count [l1] == 1
			&& def_count [l2] == 1
				replaceable [l1].rhs = l2
				erase s
	}
*/

#include "Fix_point.h"
#include <map>

class Propagate_copies : public Fix_point
{
private:
	map<string, int> use_counts;
	map<string, int> def_counts;
	map<string, HIR::Assignment*> replaceable;
	bool in_method;

public:
	Propagate_copies ();

	void pre_method (HIR::Method* in, List<HIR::Method*>* out);
	void post_method (HIR::Method* in, List<HIR::Method*>* out);
	void pre_eval_expr (HIR::Eval_expr* in, List<HIR::Statement*>* out);
};

#endif // PHC_PROPAGATE_COPIES