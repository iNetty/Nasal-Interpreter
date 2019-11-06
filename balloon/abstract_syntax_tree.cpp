#ifndef __ABSTRACT_SYNTAX_TREE_CPP__
#define __ABSTRACT_SYNTAX_TREE_CPP__

balloon_scope global;
int exit_type=0;
var abstract_syntax_tree::call_id()
{
	var temp;
	if(children.empty())
	{
		if(global.search_var(name))
			temp=global.get_var(name);
		else
		{
			std::cout<<">>[Runtime-error] cannot find a var named \'"<<name<<"\'."<<std::endl;
			exit_type=__find_var_failure;
		}
	}
	return temp;
}

var abstract_syntax_tree::get_value()
{
	var temp;
	temp.set_type(type);
	if(type==__id)
		temp=call_id();
	else if(type==__number)
		temp.set_number(number);
	else if(type==__string)
		temp.set_string(str);
	else if(type==__array)
	{
		var new_var;
		for(std::list<abstract_syntax_tree>::iterator i=children.begin();i!=children.end();++i)
		{
			new_var=i->get_value();
			temp.append_array(new_var);
		}
	}
	else if(type==__hash)
	{
		var new_var;
		for(std::list<abstract_syntax_tree>::iterator i=children.begin();i!=children.end();++i)
		{
			new_var=i->children.begin()->get_value();
			new_var.set_name(i->name);
			temp.append_array(new_var);
		}
	}
	else if(type==__function)
		temp.set_function(*this);
	else
	{
		std::cout<<">>[Runtime-error] incorrect value."<<std::endl;
		exit_type=__get_value_failure;
	}
	return temp;
}
void abstract_syntax_tree::run_root()
{
	global.set_clear();
	int beg_time,end_time;
	exit_type=__process_exited_successfully;
	beg_time=time(NULL);
	for(std::list<abstract_syntax_tree>::iterator i=children.begin();i!=children.end();++i)
	{
		if(i->type==__definition)
		{
			var new_var;
			std::list<abstract_syntax_tree>::iterator j=i->children.begin();
			std::string _name=j->name;
			if(!global.search_var(_name))
			{
				new_var.set_name(_name);
				global.add_var(new_var);
			}
			else
			{
				std::cout<<">>[Runtime-error] redeclaration of \'"<<_name<<"\'."<<std::endl;
				exit_type=__redeclaration;
				break;
			}
		}
		else if(i->type==__number)
			std::cout<<i->number<<std::endl;
		else if(i->type==__string)
			std::cout<<i->str<<std::endl;
		else if(i->type==__id)
			i->call_id();
		else if(i->type==__while)
		{
			;
		}
		else if(i->type==__ifelse)
		{
			;
		}
		
		if(exit_type!=__process_exited_successfully)
			break;
	}
	end_time=time(NULL);
	std::cout<<">>[Runtime] process exited after "<<end_time-beg_time<<" sec(s) with returned state \'";
	print_exit_type(exit_type);
	std::cout<<"\'."<<std::endl;
	return;
}


		

		

#endif
