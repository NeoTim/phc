/*
 * phc -- the open source PHP compiler
 * See doc/license/README.license for licensing information
 *
 * 
 */
#include "Plugin_pass.h"


Plugin_pass::Plugin_pass (String* name, lt_dlhandle handle, Pass_manager* pm, String* option)
{
	this->handle = handle;
	this->name = name;
	this->option = option;
}

void Plugin_pass::run (IR* in, Pass_manager* pm)
{
	typedef void (*run_function)(IR*, Pass_manager*, String*);
	/* The ltdl interface uses C, so overloading on type doesnt work. We must
	 * use different names instead. */
	if (in->is_AST ())
	{
		run_function ast_func = (run_function) lt_dlsym(handle, "run_ast");

		if (not (ast_func))
			phc_error ("Plugins which operate on the AST must define a run_ast() function.");

		(*ast_func)(in->as_AST(), pm, option);
	}
	else if (in->is_HIR ())
	{
		run_function mir_func = (run_function) lt_dlsym (handle, "run_hir");

		if (not (mir_func))
			phc_error ("Plugins which operate on the HIR must define a run_hir() function.");

		(*mir_func)(in->as_HIR (), pm, option);
	}
	else
	{
		run_function mir_func = (run_function) lt_dlsym (handle, "run_mir");

		if (not (mir_func))
			phc_error ("Plugins which operate on the MIR must define a run_mir() function.");

		(*mir_func)(in->as_MIR (), pm, option);
	}
}

void Plugin_pass::post_process ()
{
	// only do this once per instance of a pass, even though
	// it might appear at multiple locations in the pass
	// queue
	if (this->handle == NULL)
		return;

	// UNLOAD
	typedef void (*unload_function)();
	unload_function func = (unload_function) lt_dlsym(handle, "unload");

	if (func)
		(*func)();

	int ret = lt_dlclose (handle);
	if (ret != 0) 
		phc_error ("Error closing %s plugin with error: %s", name, lt_dlerror ());

	this->handle = NULL;
}


