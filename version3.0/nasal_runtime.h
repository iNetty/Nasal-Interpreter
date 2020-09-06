#ifndef __NASAL_RUNTIME_H__
#define __NASAL_RUNTIME_H__

enum runtime_returned_state
{
    rt_return=1,
    rt_break,
    rt_continue,
    rt_error,
    rt_exit_without_error
};

class nasal_runtime
{
private:
    // global_scope_address and function_return_address are addresses in garbage_collector_memory
    int function_returned_address;
    int global_scope_address;
    nasal_ast root;

    // if error occurred,this value will add 1
    int error;

    // generate number and return gc place of this number
    int number_generation(nasal_ast&);
    // generate string and return gc place of this string
    int string_generation(nasal_ast&);
    // generate vector and return gc place of this vector
    int vector_generation(nasal_ast&,int);
    // generate hash and return gc place of this hash
    int hash_generation(nasal_ast&,int);
    // generate function and return gc place of this function
    int function_generation(nasal_ast&,int);

    /*
        functions after this note may have parameter named 'local_scope_addr'
        if no local scope existing when calling these functions,use -1
    */

    // main expression block running process
    int main_progress();
    // function/loop/conditional expression block running process
    int block_progress(nasal_ast&,int,bool);
    // run loop
    int loop_progress(nasal_ast&,int,bool);
    // run conditional
    int conditional_progress(nasal_ast&,int,bool);
    // get scalars in complex data structure like vector/hash/function/closure(scope)
    int call_scalar(nasal_ast&,int);
    int call_vector(nasal_ast&,int,int);
    int call_hash(nasal_ast&,int,int);
    int call_function(nasal_ast&,int,int,int);
    // get scalars' memory place in complex data structure like vector/hash/function/closure(scope)
    int call_scalar_mem(nasal_ast&,int);
    int call_vector_mem(nasal_ast&,int,int);
    int call_hash_mem(nasal_ast&,int,int);
    // calculate scalars
    int calculation(nasal_ast&,int);
    void definition(nasal_ast&,int);
    void multi_assignment(nasal_ast&,int);
public:
    nasal_runtime();
    ~nasal_runtime();
    void set_root(nasal_ast&);
    void run();
};

nasal_runtime::nasal_runtime()
{
    error=0;
    this->root.clear();
    this->global_scope_address=-1;
    return;
}
nasal_runtime::~nasal_runtime()
{
    error=0;
    this->root.clear();
    this->global_scope_address=-1;
    return;
}
void nasal_runtime::set_root(nasal_ast& parse_result)
{
    this->root=parse_result;
    return;
}
void nasal_runtime::run()
{
    int returned_statement;
    time_t begin_time,end_time;
    this->global_scope_address=nasal_vm.gc_alloc();
    nasal_vm.gc_get(global_scope_address).set_type(vm_closure);
    nasal_vm.gc_get(global_scope_address).get_closure().add_scope();
    begin_time=std::time(NULL);
    returned_statement=main_progress();
    end_time=std::time(NULL);
    nasal_vm.gc_get(global_scope_address).get_closure().del_scope();
    nasal_vm.del_reference(global_scope_address);
    std::cout<<">> [runtime] process exited after "<<end_time-begin_time<<"s with returned value "<<returned_statement<<'.'<<std::endl;
    return;
}

// private functions
int nasal_runtime::number_generation(nasal_ast& node)
{
    int new_addr=nasal_vm.gc_alloc();
    nasal_vm.gc_get(new_addr).set_type(vm_number);
    nasal_vm.gc_get(new_addr).set_number(trans_string_to_number(node.get_str()));
    return new_addr;
}
int nasal_runtime::string_generation(nasal_ast& node)
{
    int new_addr=nasal_vm.gc_alloc();
    nasal_vm.gc_get(new_addr).set_type(vm_string);
    nasal_vm.gc_get(new_addr).set_string(node.get_str());
    return new_addr;
}
int nasal_runtime::vector_generation(nasal_ast& node,int local_scope_addr)
{
    int new_addr=nasal_vm.gc_alloc();
    nasal_vm.gc_get(new_addr).set_type(vm_vector);
    nasal_vector& ref_of_this_vector=nasal_vm.gc_get(new_addr).get_vector();

    int node_children_size=node.get_children().size();
    for(int i=0;i<node_children_size;++i)
    {
        int new_memory_space=nasal_vm.mem_alloc();
        ref_of_this_vector.add_elem(new_memory_space);
        switch(node.get_children()[i].get_type())
        {
            case ast_number:   nasal_vm.mem_init(new_memory_space,number_generation(node.get_children()[i]));break;
            case ast_string:   nasal_vm.mem_init(new_memory_space,string_generation(node.get_children()[i]));break;
            case ast_vector:   nasal_vm.mem_init(new_memory_space,vector_generation(node.get_children()[i],local_scope_addr));break;
            case ast_hash:     nasal_vm.mem_init(new_memory_space,hash_generation(node.get_children()[i],local_scope_addr));break;
            case ast_function: nasal_vm.mem_init(new_memory_space,function_generation(node.get_children()[i],local_scope_addr));break;
        }
    }
    return new_addr;
}
int nasal_runtime::hash_generation(nasal_ast& node,int local_scope_addr)
{
    int new_addr=nasal_vm.gc_alloc();
    nasal_vm.gc_get(new_addr).set_type(vm_hash);
    nasal_hash& ref_of_this_hash=nasal_vm.gc_get(new_addr).get_hash();

    int node_children_size=node.get_children().size();
    for(int i=0;i<node_children_size;++i)
    {
        int new_memory_space=nasal_vm.mem_alloc();
        std::string hash_member_name=node.get_children()[i].get_children()[0].get_str();
        ref_of_this_hash.add_elem(hash_member_name,new_memory_space);
        nasal_ast& ref_of_hash_member_value=node.get_children()[i].get_children()[1];
        switch(ref_of_hash_member_value.get_type())
        {
            case ast_number:   nasal_vm.mem_init(new_memory_space,number_generation(ref_of_hash_member_value));break;
            case ast_string:   nasal_vm.mem_init(new_memory_space,string_generation(ref_of_hash_member_value));break;
            case ast_vector:   nasal_vm.mem_init(new_memory_space,vector_generation(ref_of_hash_member_value,local_scope_addr));break;
            case ast_hash:     nasal_vm.mem_init(new_memory_space,hash_generation(ref_of_hash_member_value,local_scope_addr));break;
            case ast_function: nasal_vm.mem_init(new_memory_space,function_generation(ref_of_hash_member_value,local_scope_addr));break;
        }
    }
    return new_addr;
}
int nasal_runtime::function_generation(nasal_ast& node,int local_scope_addr)
{
    int new_addr=nasal_vm.gc_alloc();
    nasal_vm.gc_get(new_addr).set_type(vm_function);
    nasal_function& ref_of_this_function=nasal_vm.gc_get(new_addr).get_func();

    ref_of_this_function.set_arguments(node.get_children()[0]);
    ref_of_this_function.set_run_block(node.get_children()[1]);
    if(local_scope_addr>=0)
    {
        // codes here make closure
        nasal_vm.add_reference(local_scope_addr);
        ref_of_this_function.set_closure_addr(local_scope_addr);
    }
    else
    {
        int new_closure_addr=nasal_vm.gc_alloc();
        nasal_vm.gc_get(new_closure_addr).set_type(vm_closure);
        ref_of_this_function.set_closure_addr(new_closure_addr);
    }
    return new_addr;
}
int nasal_runtime::main_progress()
{
    int ret_state=rt_exit_without_error;
    int expr_number=root.get_children().size();
    int process_returned_value_addr=-1;
    for(int i=0;i<expr_number;++i)
    {
        int node_type=root.get_children()[i].get_type();
        switch(node_type)
        {
            case ast_definition:
                definition(root.get_children()[i],-1);
                break;
            case ast_multi_assign:
                multi_assignment(root.get_children()[i],-1);
                break;
            case ast_conditional:
                ret_state=conditional_progress(root.get_children()[i],-1,false);
                break;
            case ast_while:
            case ast_for:
            case ast_forindex:
            case ast_foreach:
                ret_state=loop_progress(root.get_children()[i],-1,false);
                break;
            case ast_number:break;
            case ast_string:break;
            case ast_identifier:break;
            case ast_call:
            case ast_add_equal:
            case ast_sub_equal:
            case ast_mult_equal:
            case ast_div_equal:
            case ast_link_equal:
            case ast_unary_sub:
            case ast_unary_not:
            case ast_add:
            case ast_sub:
            case ast_mult:
            case ast_div:
            case ast_link:
            case ast_trinocular:
                process_returned_value_addr=calculation(root.get_children()[i],-1);
                nasal_vm.del_reference(process_returned_value_addr);
                break;
            case ast_break:
                ret_state=rt_break;
                break;
            case ast_continue:
                ret_state=rt_continue;
                break;
            case ast_return:
                ret_state=rt_return;
                break;
        }
        switch(ret_state)
        {
            case rt_break:
                std::cout<<">> [runtime] main_progress: cannot use break in main progress."<<std::endl;
                ++error;
                break;
            case rt_continue:
                std::cout<<">> [runtime] main_progress: cannot use continue in main progress."<<std::endl;
                ++error;
                break;
            case rt_return:
                std::cout<<">> [runtime] main_progress: cannot use return in main progress."<<std::endl;
                ++error;
                break;
            case rt_error:
                std::cout<<">> [runtime] main_progress: error occurred when executing main progress."<<std::endl;
                ++error;
                break;
        }
        if(error)
        {
            ret_state=rt_error;
            break;
        }
    }
    return ret_state;
}
int nasal_runtime::block_progress(nasal_ast& node,int local_scope_addr,bool allow_return)
{
    int ret_state=rt_exit_without_error;
    if(local_scope_addr<0)
    {
        local_scope_addr=nasal_vm.gc_alloc();
        nasal_vm.gc_get(local_scope_addr).set_type(vm_closure);
        nasal_vm.gc_get(local_scope_addr).get_closure().add_scope();
    }
    else
        nasal_vm.add_reference(local_scope_addr);
    int expr_number=node.get_children().size();
    int process_returned_value_addr=-1;
    for(int i=0;i<expr_number;++i)
    {
        int node_type=node.get_children()[i].get_type();
        switch(node_type)
        {
            case ast_definition:
                definition(node.get_children()[i],local_scope_addr);
                break;
            case ast_multi_assign:
                multi_assignment(node.get_children()[i],local_scope_addr);
                break;
            case ast_conditional:
                ret_state=conditional_progress(node.get_children()[i],local_scope_addr,allow_return);
                break;
            case ast_while:
            case ast_for:
            case ast_forindex:
            case ast_foreach:
                ret_state=loop_progress(node.get_children()[i],local_scope_addr,allow_return);
                break;
            case ast_number:break;
            case ast_string:break;
            case ast_identifier:break;
            case ast_call:
            case ast_add_equal:
            case ast_sub_equal:
            case ast_mult_equal:
            case ast_div_equal:
            case ast_link_equal:
            case ast_unary_sub:
            case ast_unary_not:
            case ast_add:
            case ast_sub:
            case ast_mult:
            case ast_div:
            case ast_link:
            case ast_trinocular:
                process_returned_value_addr=calculation(root.get_children()[i],local_scope_addr);
                nasal_vm.del_reference(process_returned_value_addr);
                break;
            case ast_break:
                ret_state=rt_break;
                break;
            case ast_continue:
                ret_state=rt_continue;
                break;
            case ast_return:
                ret_state=rt_return;
                if(allow_return)
                    function_returned_address=calculation(root.get_children()[i].get_children()[0],local_scope_addr);
                else
                {
                    std::cout<<">> [runtime] return expression is not allowed here."<<std::endl;
                    ++error;
                }
                break;
        }
        if(ret_state==rt_error)
        {
            std::cout<<">> [runtime] block_progress: error occurred when executing sub-progress."<<std::endl;
            ++error;
        }
        if(error || ret_state==rt_break || ret_state==rt_continue || ret_state==rt_return)
            break;
    }
    nasal_vm.del_reference(local_scope_addr);
    return ret_state;
}
int nasal_runtime::loop_progress(nasal_ast& node,int local_scope_addr,bool allow_return)
{
    int ret_state=rt_exit_without_error;
    // unfinished
    return ret_state;
}
int nasal_runtime::conditional_progress(nasal_ast& node,int local_scope_addr,bool allow_return)
{
    int ret_state=rt_exit_without_error;
    // unfinished
    return ret_state;
}
int nasal_runtime::call_scalar(nasal_ast& node,int local_scope_addr)
{
    int value_address=-1;
    if(local_scope_addr>=0)
        value_address=nasal_vm.gc_get(local_scope_addr).get_closure().get_value_address(node.get_children()[0].get_str());
    if(value_address<0)
        value_address=nasal_vm.gc_get(global_scope_address).get_closure().get_value_address(node.get_children()[0].get_str());
    if(value_address<0)
    {
        // unfinished
        // builtin-function call will be set here
        std::cout<<">> [runtime] call_nasal_scalar: cannot find value named \'"<<node.get_children()[0].get_str()<<"\'."<<std::endl;
        ++error;
        return -1;
    }
    nasal_vm.add_reference(value_address);
    int call_expr_size=node.get_children().size();
    int last_call_hash_addr=-1;
    for(int i=1;i<call_expr_size;++i)
    {
        int tmp_value_addr=-1;
        nasal_ast& call_expr=node.get_children()[i];
        switch(call_expr.get_type())
        {
            case ast_call_vec:
                tmp_value_addr=call_vector(call_expr,value_address,local_scope_addr);
                last_call_hash_addr=-1;
                break;
            case ast_call_hash:
                tmp_value_addr=call_hash(call_expr,value_address,local_scope_addr);
                last_call_hash_addr=value_address;
                break;
            case ast_call_func:
                tmp_value_addr=call_function(call_expr,value_address,last_call_hash_addr,local_scope_addr);
                last_call_hash_addr=-1;
                break;
        }
        nasal_vm.del_reference(value_address);
        value_address=tmp_value_addr;
        if(value_address<0)
            break;
    }
    return value_address;
}
int nasal_runtime::call_vector(nasal_ast& node,int base_value_addr,int local_scope_addr)
{
    int return_value_addr=-1;
    int base_value_type=nasal_vm.gc_get(base_value_addr).get_type();
    if(base_value_type!=vm_vector && base_value_type!=vm_hash && base_value_type!=vm_string)
    {
        std::cout<<">> [runtime] call_vector: incorrect value type,expected a vector/hash/string."<<std::endl;
        ++error;
        return -1;
    }
    int call_size=node.get_children().size();

    if(base_value_type==vm_vector)
    {
        std::vector<int> called_value_addrs;
        nasal_vector& reference_value=nasal_vm.gc_get(base_value_addr).get_vector();
        for(int i=0;i<call_size;++i)
        {
            if(node.get_children()[i].get_type()==ast_subvec)
            {
                nasal_ast& subvec_node=node.get_children()[i];
                int begin_value_addr=calculation(subvec_node.get_children()[0],local_scope_addr);
                int end_value_addr=calculation(subvec_node.get_children()[1],local_scope_addr);
                int begin_value_type=nasal_vm.gc_get(begin_value_addr).get_type();
                int end_value_type=nasal_vm.gc_get(end_value_addr).get_type();
                bool begin_is_nil=true,end_is_nil=true;
                int begin_index=0,end_index=0;
                if(begin_value_type!=vm_nil && begin_value_type!=vm_number && begin_value_type!=vm_string)
                {
                    std::cout<<">> [runtime] call_vector: begin index is not a number/numerable string."<<std::endl;
                    ++error;
                    return -1;
                }
                if(end_value_type!=vm_nil && end_value_type!=vm_number && end_value_type!=vm_string)
                {
                    std::cout<<">> [runtime] call_vector: end index is not a number/numerable string."<<std::endl;
                    ++error;
                    return -1;
                }
                if(begin_value_type==vm_string)
                {
                    std::string str=nasal_vm.gc_get(begin_value_addr).get_string();
                    if(!check_numerable_string(str))
                    {
                        std::cout<<">> [runtime] call_vector: begin index is not a numerable string."<<std::endl;
                        ++error;
                        return -1;
                    }
                    begin_index=(int)trans_string_to_number(str);
                    begin_is_nil=false;
                }
                else if(begin_value_type==vm_number)
                {
                    begin_index=(int)nasal_vm.gc_get(begin_value_addr).get_number();
                    begin_is_nil=false;
                }
                
                if(end_value_type==vm_string)
                {
                    std::string str=nasal_vm.gc_get(end_value_addr).get_string();
                    if(!check_numerable_string(str))
                    {
                        std::cout<<">> [runtime] call_vector: end index is not a numerable string."<<std::endl;
                        ++error;
                        return -1;
                    }
                    end_index=(int)trans_string_to_number(str);
                    end_is_nil=false;
                }
                else if(end_value_type==vm_number)
                {
                    end_index=(int)nasal_vm.gc_get(end_value_addr).get_number();
                    end_is_nil=false;
                }
                
                if(begin_is_nil && end_is_nil)
                {
                    begin_index=0;
                    end_index=reference_value.size()-1;
                }
                else if(begin_is_nil && !end_is_nil)
                {
                    begin_index=end_index<0? -reference_value.size():0;
                }
                else if(!begin_is_nil && end_is_nil)
                {
                    end_index=begin_index<0? -1:reference_value.size()-1;
                }
                else if(!begin_is_nil && !end_is_nil)
                {
                    if(begin_index>=end_index)
                    {
                        std::cout<<">> [runtime] call_vector: begin index must be less than end index."<<std::endl;
                        ++error;
                        return -1;
                    }
                }
                for(int i=begin_index;i<end_index;++i)
                    called_value_addrs.push_back(reference_value.get_value_address(i));
                nasal_vm.del_reference(begin_value_addr);
                nasal_vm.del_reference(end_value_addr);
            }
            else
            {
                int index_value_addr=calculation(node.get_children()[i],local_scope_addr);
                int index_value_type=nasal_vm.gc_get(index_value_addr).get_type();
                if(index_value_type!=vm_number && index_value_type!=vm_string)
                {
                    std::cout<<">> [runtime] call_vector: index is not a number/numerable string."<<std::endl;
                    ++error;
                    return -1;
                }
                int index_num=0;
                if(index_value_type==vm_string)
                {
                    std::string str=nasal_vm.gc_get(index_value_addr).get_string();
                    if(!check_numerable_string(str))
                    {
                        std::cout<<">> [runtime] call_vector: index is not a numerable string."<<std::endl;
                        ++error;
                        return -1;
                    }
                    index_num=(int)trans_string_to_number(str);
                }
                else
                    index_num=(int)nasal_vm.gc_get(index_value_addr).get_number();
                nasal_vm.del_reference(index_value_addr);
                called_value_addrs.push_back(reference_value.get_value_address(index_num));
            }
        }
        if(called_value_addrs.size()==1)
        {
            int value_addr=called_value_addrs[0];
            int value_type=nasal_vm.gc_get(value_addr).get_type();
            if(value_type==vm_vector || value_type==vm_hash)
            {
                nasal_vm.add_reference(value_addr);
                return_value_addr=value_addr;
            }
            else
            {
                return_value_addr=nasal_vm.gc_alloc();
                nasal_vm.gc_get(return_value_addr).deepcopy(nasal_vm.gc_get(value_addr));
            }
        }
        else
        {
            return_value_addr=nasal_vm.gc_alloc();
            nasal_vm.gc_get(return_value_addr).set_type(vm_vector);
            nasal_vector& return_vector=nasal_vm.gc_get(return_value_addr).get_vector();
            int vec_size=called_value_addrs.size();
            for(int i=0;i<vec_size;++i)
            {
                int value_addr=called_value_addrs[i];
                int value_type=nasal_vm.gc_get(value_addr).get_type();
                if(value_type==vm_vector || value_type==vm_hash)
                {
                    nasal_vm.add_reference(value_addr);
                    int new_mem_addr=nasal_vm.mem_alloc();
                    nasal_vm.mem_init(new_mem_addr,value_addr);
                    return_vector.add_elem(new_mem_addr);
                }
                else
                {
                    int tmp_value_addr=nasal_vm.gc_alloc();
                    nasal_vm.gc_get(tmp_value_addr).deepcopy(nasal_vm.gc_get(value_addr));
                    int new_mem_addr=nasal_vm.mem_alloc();
                    nasal_vm.mem_init(new_mem_addr,tmp_value_addr);
                    return_vector.add_elem(new_mem_addr);
                }
            }
        }
    }
    else if(base_value_type==vm_hash)
    {
        if(call_size>1)
        {
            std::cout<<">> [runtime] call_vector: when calling a hash,only one key is alowed."<<std::endl;
            ++error;
            return -1;
        }
        if(node.get_children()[0].get_type()==ast_subvec)
        {
            std::cout<<">> [runtime] call_vector: cannot slice hash."<<std::endl;
            ++error;
            return -1;
        }
        std::string str=node.get_children()[0].get_str();
        int value_addr=nasal_vm.gc_get(base_value_addr).get_hash().get_value_address(str);
        int value_type=nasal_vm.gc_get(value_addr).get_type();
        if(value_type==vm_vector || value_type==vm_hash)
        {
            nasal_vm.add_reference(value_addr);
            return_value_addr=value_addr;
        }
        else
        {
            return_value_addr=nasal_vm.gc_alloc();
            nasal_vm.gc_get(return_value_addr).deepcopy(nasal_vm.gc_get(value_addr));
        }
    }
    else
    {
        if(call_size>1)
        {
            std::cout<<">> [runtime] call_vector: when calling a string,only one index is alowed."<<std::endl;
            ++error;
            return -1;
        }
        if(node.get_children()[0].get_type()==ast_subvec)
        {
            std::cout<<">> [runtime] call_vector: cannot slice string."<<std::endl;
            ++error;
            return -1;
        }
        int index_value_addr=calculation(node.get_children()[0],local_scope_addr);
        int index_value_type=nasal_vm.gc_get(index_value_addr).get_type();
        if(index_value_type!=vm_number && index_value_type!=vm_string)
        {
            std::cout<<">> [runtime] call_vector: index is not a number/numerable string."<<std::endl;
            ++error;
            return -1;
        }
        int index_num=0;
        if(index_value_type==vm_string)
        {
            std::string str=nasal_vm.gc_get(index_value_addr).get_string();
            if(!check_numerable_string(str))
            {
                std::cout<<">> [runtime] call_vector: index is not a numerable string."<<std::endl;
                ++error;
                return -1;
            }
            index_num=(int)trans_string_to_number(str);
        }
        else
            index_num=(int)nasal_vm.gc_get(index_value_addr).get_number();
        nasal_vm.del_reference(index_value_addr);
        return_value_addr=nasal_vm.gc_alloc();
        nasal_vm.gc_get(return_value_addr).set_type(vm_number);
        std::string str=nasal_vm.gc_get(base_value_addr).get_string();
        int str_size=str.length();
        if(index_num>=str_size || index_num<-str_size)
        {
            std::cout<<">> [runtime] call_vector: index out of range."<<std::endl;
            ++error;
            return -1;
        }
        nasal_vm.gc_get(return_value_addr).set_number((double)str[(index_num+str_size)%str_size]);
    }
    return return_value_addr;
}
int nasal_runtime::call_hash(nasal_ast& node,int base_value_addr,int local_scope_addr)
{
    int value_type=nasal_vm.gc_get(base_value_addr).get_type();
    if(value_type!=vm_hash)
    {
        std::cout<<">> [runtime] call_hash: incorrect value type,expected a hash."<<std::endl;
        ++error;
        return -1;
    }
    int ret_value_addr=nasal_vm.gc_get(base_value_addr).get_hash().get_value_address(node.get_str());
    nasal_vm.add_reference(ret_value_addr);
    return ret_value_addr;
}
int nasal_runtime::call_function(nasal_ast& node,int base_value_addr,int last_call_hash_addr,int local_scope_addr)
{
    int ret_value_addr=-1;
    int value_type=nasal_vm.gc_get(base_value_addr).get_type();
    if(value_type!=vm_function)
    {
        std::cout<<">> [runtime] call_function: incorrect value type,expected a function."<<std::endl;
        return -1;
    }
    nasal_function& reference_of_func=nasal_vm.gc_get(base_value_addr).get_func();
    int run_closure_addr=reference_of_func.get_closure_addr();
    nasal_closure& run_closure=nasal_vm.gc_get(run_closure_addr).get_closure();
    run_closure.add_scope();
    nasal_vm.add_reference(last_call_hash_addr);
    run_closure.add_new_value("me",last_call_hash_addr);
    nasal_ast& argument_format=reference_of_func.get_arguments();
    if(!node.get_children().size())
    {
        if(argument_format.get_children().size() && argument_format.get_children()[0].get_type()!=ast_default_arg && argument_format.get_children()[0].get_type()!=ast_dynamic_id)
        {
            int size=argument_format.get_children().size();
            int sum=0;
            for(int i=0;i<size;++i)
                sum+=(argument_format.get_children()[i].get_type()!=ast_default_arg);
            std::cout<<">> [runtime] call_function: lack at least "<<sum<<" argument(s) but get 0."<<std::endl;
            ++error;
            return -1;
        }
        else if(argument_format.get_children().size() && argument_format.get_children()[0].get_type()==ast_dynamic_id)
        {
            // load null dynamic-id
            int vector_value_addr=nasal_vm.gc_alloc();
            nasal_vm.gc_get(vector_value_addr).set_type(vm_vector);
            run_closure.add_new_value(argument_format.get_children()[0].get_str(),vector_value_addr);
        }
        else if(argument_format.get_children().size() && argument_format.get_children()[0].get_type()==ast_default_arg)
        {
            // load default values
            int size=argument_format.get_children().size();
            for(int i=0;i<size;++i)
            {
                int tmp_value_addr=calculation(argument_format.get_children()[i].get_children()[1],local_scope_addr);
                if(tmp_value_addr<0)
                    return -1;
                run_closure.add_new_value(argument_format.get_children()[i].get_children()[0].get_str(),tmp_value_addr);
            }
        }
    }
    else if(node.get_children()[0].get_type()==ast_hashmember)
    {
        std::map<std::string,bool> args_usage_table;        // check arguments in argument_format is correctly used
        std::map<std::string,bool> default_args_table;      // check default arguments
        std::map<std::string,nasal_ast*> default_args_node; // if one of default arguments is not in use,use default value
        // load arguments' name.
        int arg_format_size=argument_format.get_children().size();
        for(int i=0;i<arg_format_size;++i)
        {
            nasal_ast& tmp_node=argument_format.get_children()[i];
            std::string id_name=(tmp_node.get_type()==ast_default_arg? tmp_node.get_children()[0].get_str():tmp_node.get_str());
            args_usage_table[id_name]=false;
            if(tmp_node.get_type()==ast_default_arg)
            {
                default_args_table[id_name]=false;
                default_args_node[id_name]=&tmp_node.get_children()[1];
            }
        }
        // initialize normal arguments.
        int args_size=node.get_children().size();
        for(int i=0;i<args_size;++i)
        {
            nasal_ast& tmp_node=node.get_children()[i];
            std::string id_name=tmp_node.get_children()[0].get_str();
            if(args_usage_table.find(id_name)==args_usage_table.end())
            {
                std::cout<<">> [runtime] call_function: identifier named \'"<<id_name<<"\' does not exist."<<std::endl;
                ++error;
                return -1;
            }
            else
            {
                args_usage_table[id_name]=true;
                if(default_args_table.find(id_name)!=default_args_table.end())
                    default_args_table[id_name]=true;
            }
            int value_addr=calculation(tmp_node.get_children()[1],local_scope_addr);
            if(value_addr<0)
                return -1;
            run_closure.add_new_value(id_name,value_addr);
        }
        // use default value if a default-argument haven't been initialized.
        for(std::map<std::string,bool>::iterator i=default_args_table.begin();i!=default_args_table.end();++i)
            if(!i->second)
            {
                int value_addr=calculation(*default_args_node[i->first],local_scope_addr);
                if(value_addr<0)
                    return -1;
                run_closure.add_new_value(i->first,value_addr);
                args_usage_table[i->first]=true;
            }
        // use null vector if dynamic-identifier haven't been initialized.
        if(argument_format.get_children().back().get_type()==ast_dynamic_id)
        {
            std::string dyn_str=argument_format.get_children().back().get_str();
            if(!args_usage_table[dyn_str])
            {
                args_usage_table[dyn_str]=true;
                int vector_value_addr=nasal_vm.gc_alloc();
                nasal_vm.gc_get(vector_value_addr).set_type(vm_vector);
                run_closure.add_new_value(dyn_str,vector_value_addr);
            }
        }
        // check if each argument is initialized.
        for(std::map<std::string,bool>::iterator i=args_usage_table.begin();i!=args_usage_table.end();++i)
            if(!i->second)
            {
                std::cout<<">> [runtime] call_function: argument named \'"<<i->first<<"\' is not in use."<<std::endl;
                ++error;
                return -1;
            }
    }
    else
    {
        std::vector<int> args; // store value address of input arguments
        int size=node.get_children().size();
        for(int i=0;i<size;++i)
        {
            int tmp_val_addr=calculation(node.get_children()[i],local_scope_addr);
            if(tmp_val_addr<0)
            {
                std::cout<<">> [runtime] call_function: error value address when generating argument list."<<std::endl;
                ++error;
                return -1;
            }
            args.push_back(tmp_val_addr);
        }
        int arg_format_size=argument_format.get_children().size();
        if(size>arg_format_size && argument_format.get_children().back().get_type()!=ast_dynamic_id)
        {
            std::cout<<">> [runtime] call_function: too much arguments."<<std::endl;
            ++error;
            return -1;
        }
        for(int i=0;i<arg_format_size;++i)
        {
            nasal_ast& tmp_node=argument_format.get_children()[i];
            if(tmp_node.get_type()==ast_identifier || tmp_node.get_type()==ast_dynamic_id)
            {
                if(tmp_node.get_type()==ast_dynamic_id)
                {
                    int vector_value_addr=nasal_vm.gc_alloc();
                    nasal_vm.gc_get(vector_value_addr).set_type(vm_vector);
                    nasal_vector& ref_vec=nasal_vm.gc_get(vector_value_addr).get_vector();
                    for(int j=i;j<size;++j)
                    {
                        int new_mem=nasal_vm.mem_alloc();
                        nasal_vm.mem_init(new_mem,args[j]);
                        ref_vec.add_elem(new_mem);
                    }
                    run_closure.add_new_value(tmp_node.get_str(),vector_value_addr);
                    break;
                }
                if(i<size)
                    run_closure.add_new_value(tmp_node.get_str(),args[i]);
                else
                {
                    std::cout<<">> [runtime] call_function: lack argument(s).stop."<<std::endl;
                    ++error;
                    return -1;
                }
            }
            else // default_args
            {
                if(i<size)
                    run_closure.add_new_value(tmp_node.get_children()[0].get_str(),args[i]);
                else
                {
                    int tmp_val_addr=calculation(tmp_node.get_children()[1],local_scope_addr);
                    if(tmp_val_addr<0)
                        return -1;
                    run_closure.add_new_value(tmp_node.get_children()[0].get_str(),tmp_val_addr);
                }
            }
        }
    }
    block_progress(reference_of_func.get_run_block(),run_closure_addr,true);

    run_closure.del_scope();
    if(function_returned_address>=0)
    {
        ret_value_addr=function_returned_address;
        function_returned_address=-1;
    }
    else
    {
        ret_value_addr=nasal_vm.gc_alloc();
        nasal_vm.gc_get(ret_value_addr).set_type(vm_nil);
    }
    return ret_value_addr;
}
int nasal_runtime::call_scalar_mem(nasal_ast& node,int local_scope_addr)
{
    int mem_address=-1;
    if(local_scope_addr>=0)
        mem_address=nasal_vm.gc_get(local_scope_addr).get_closure().get_mem_address(node.get_children()[0].get_str());
    if(mem_address<0)
        mem_address=nasal_vm.gc_get(global_scope_address).get_closure().get_mem_address(node.get_children()[0].get_str());
    if(mem_address<0)
    {
        std::cout<<">> [runtime] call_scalar_mem: cannot find value named \'"<<node.get_children()[0].get_str()<<"\'."<<std::endl;
        ++error;
        return -1;
    }
    int call_expr_size=node.get_children().size();
    for(int i=1;i<call_expr_size;++i)
    {
        int tmp_mem_addr=-1;
        nasal_ast& call_expr=node.get_children()[i];
        switch(call_expr.get_type())
        {
            case ast_call_vec:  tmp_mem_addr=call_vector_mem(call_expr,mem_address,local_scope_addr);break;
            case ast_call_hash: tmp_mem_addr=call_hash_mem(call_expr,mem_address,local_scope_addr);break;
            case ast_call_func:
                std::cout<<">> [runtime] call_scalar_mem: cannot change the value that function returns."<<std::endl;
                ++error;
                return -1;
                break;
        }
        mem_address=tmp_mem_addr;
        if(mem_address<0)
            break;
    }
    return mem_address;
}
int nasal_runtime::call_vector_mem(nasal_ast& node,int base_mem_addr,int local_scope_addr)
{
    int return_mem_addr=-1;
    int base_value_addr=nasal_vm.mem_get(base_mem_addr);
    int base_value_type=nasal_vm.gc_get(base_value_addr).get_type();
    if(base_value_type!=vm_vector && base_value_type!=vm_hash)
    {
        std::cout<<">> [runtime] call_vector_mem: incorrect value type,expected a vector/hash."<<std::endl;
        ++error;
        return -1;
    }
    int call_size=node.get_children().size();
    if(call_size>1)
    {
        std::cout<<">> [runtime] call_vector_mem: when searching a memory space in a vector,only one index is alowed."<<std::endl;
        ++error;
        return -1;
    }
    if(base_value_type==vm_vector)
    {
        nasal_vector& reference_value=nasal_vm.gc_get(base_value_addr).get_vector();
        if(node.get_children()[0].get_type()==ast_subvec)
        {
            std::cout<<">> [runtime] call_vector_mem: sub-vector in this progress is a temporary value and cannot be changed."<<std::endl;
            ++error;
            return -1;
        }
        int index_value_addr=calculation(node.get_children()[0],local_scope_addr);
        int index_value_type=nasal_vm.gc_get(index_value_addr).get_type();
        if(index_value_type!=vm_number && index_value_type!=vm_string)
        {
            std::cout<<">> [runtime] call_vector_mem: index is not a number/numerable string."<<std::endl;
            ++error;
            return -1;
        }
        int index_num=0;
        if(index_value_type==vm_string)
        {
            std::string str=nasal_vm.gc_get(index_value_addr).get_string();
            if(!check_numerable_string(str))
            {
                std::cout<<">> [runtime] call_vector_mem: index is not a numerable string."<<std::endl;
                ++error;
                return -1;
            }
            index_num=(int)trans_string_to_number(str);
        }
        else
            index_num=(int)nasal_vm.gc_get(index_value_addr).get_number();
        nasal_vm.del_reference(index_value_addr);
        return_mem_addr=reference_value.get_mem_address(index_num);
    }
    else
    {
        if(call_size>1)
        {
            std::cout<<">> [runtime] call_vector_mem: when calling a hash,only one key is alowed."<<std::endl;
            ++error;
            return -1;
        }
        std::string str=node.get_children()[0].get_str();
        return_mem_addr=nasal_vm.gc_get(base_value_addr).get_hash().get_mem_address(str);
    }
    return return_mem_addr;
}
int nasal_runtime::call_hash_mem(nasal_ast& node,int base_mem_addr,int local_scope_addr)
{
    int base_value_addr=nasal_vm.mem_get(base_mem_addr);
    int value_type=nasal_vm.gc_get(base_value_addr).get_type();
    if(value_type!=vm_hash)
    {
        std::cout<<">> [runtime] call_hash_mem: incorrect value type,expected a hash."<<std::endl;
        ++error;
        return -1;
    }
    int ret_mem_addr=nasal_vm.gc_get(base_value_addr).get_hash().get_mem_address(node.get_str());
    return ret_mem_addr;
}
int nasal_runtime::calculation(nasal_ast& node,int local_scope_addr)
{
    // after this process, a new address(in nasal_vm.garbage_collector_memory) will be returned
    int ret_address=-1;
    int calculation_type=node.get_type();
    if(calculation_type==ast_number)
        ret_address=number_generation(node);
    else if(calculation_type==ast_identifier)
    {
        if(local_scope_addr>=0)
            ret_address=nasal_vm.gc_get(local_scope_addr).get_closure().get_value_address(node.get_str());
        if(ret_address<0)
            ret_address=nasal_vm.gc_get(global_scope_address).get_closure().get_value_address(node.get_str());
        if(ret_address<0)
        {
            std::cout<<">> [runtime] calculation: cannot find value named \'"<<node.get_str()<<"\'."<<std::endl;
            ++error;
        }
    }
    else if(calculation_type==ast_string)
        ret_address=string_generation(node);
    else if(calculation_type==ast_vector)
        ret_address=vector_generation(node,local_scope_addr);
    else if(calculation_type==ast_hash)
        ret_address=hash_generation(node,local_scope_addr);
    else if(calculation_type==ast_function)
        ret_address=function_generation(node,local_scope_addr);
    else if(calculation_type==ast_call)
        ret_address=call_scalar(node,local_scope_addr);
    else if(calculation_type==ast_add)
    {
        int left_gc_addr=calculation(node.get_children()[0],local_scope_addr);
        int right_gc_addr=calculation(node.get_children()[1],local_scope_addr);
        ret_address=nasal_scalar_calculator.nasal_scalar_add(left_gc_addr,right_gc_addr);
        // delete the reference of temporary values
        nasal_vm.del_reference(left_gc_addr);
        nasal_vm.del_reference(right_gc_addr);
    }
    else if(calculation_type==ast_sub)
    {
        int left_gc_addr=calculation(node.get_children()[0],local_scope_addr);
        int right_gc_addr=calculation(node.get_children()[1],local_scope_addr);
        ret_address=nasal_scalar_calculator.nasal_scalar_sub(left_gc_addr,right_gc_addr);
        // delete the reference of temporary values
        nasal_vm.del_reference(left_gc_addr);
        nasal_vm.del_reference(right_gc_addr);
    }
    else if(calculation_type==ast_mult)
    {
        int left_gc_addr=calculation(node.get_children()[0],local_scope_addr);
        int right_gc_addr=calculation(node.get_children()[1],local_scope_addr);
        ret_address=nasal_scalar_calculator.nasal_scalar_mult(left_gc_addr,right_gc_addr);
        // delete the reference of temporary values
        nasal_vm.del_reference(left_gc_addr);
        nasal_vm.del_reference(right_gc_addr);
    }
    else if(calculation_type==ast_div)
    {
        int left_gc_addr=calculation(node.get_children()[0],local_scope_addr);
        int right_gc_addr=calculation(node.get_children()[1],local_scope_addr);
        ret_address=nasal_scalar_calculator.nasal_scalar_div(left_gc_addr,right_gc_addr);
        // delete the reference of temporary values
        nasal_vm.del_reference(left_gc_addr);
        nasal_vm.del_reference(right_gc_addr);
    }
    else if(calculation_type==ast_link)
    {
        int left_gc_addr=calculation(node.get_children()[0],local_scope_addr);
        int right_gc_addr=calculation(node.get_children()[1],local_scope_addr);
        ret_address=nasal_scalar_calculator.nasal_scalar_link(left_gc_addr,right_gc_addr);
        // delete the reference of temporary values
        nasal_vm.del_reference(left_gc_addr);
        nasal_vm.del_reference(right_gc_addr);
    }
    else if(calculation_type==ast_cmp_equal)
    {
        int left_gc_addr=calculation(node.get_children()[0],local_scope_addr);
        int right_gc_addr=calculation(node.get_children()[1],local_scope_addr);
        ret_address=nasal_scalar_calculator.nasal_scalar_cmp_equal(left_gc_addr,right_gc_addr);
        // delete the reference of temporary values
        nasal_vm.del_reference(left_gc_addr);
        nasal_vm.del_reference(right_gc_addr);
    }
    else if(calculation_type==ast_cmp_not_equal)
    {
        int left_gc_addr=calculation(node.get_children()[0],local_scope_addr);
        int right_gc_addr=calculation(node.get_children()[1],local_scope_addr);
        ret_address=nasal_scalar_calculator.nasal_scalar_cmp_not_equal(left_gc_addr,right_gc_addr);
        // delete the reference of temporary values
        nasal_vm.del_reference(left_gc_addr);
        nasal_vm.del_reference(right_gc_addr);
    }
    else if(calculation_type==ast_less_than)
    {
        int left_gc_addr=calculation(node.get_children()[0],local_scope_addr);
        int right_gc_addr=calculation(node.get_children()[1],local_scope_addr);
        ret_address=nasal_scalar_calculator.nasal_scalar_cmp_less(left_gc_addr,right_gc_addr);
        // delete the reference of temporary values
        nasal_vm.del_reference(left_gc_addr);
        nasal_vm.del_reference(right_gc_addr);
    }
    else if(calculation_type==ast_less_equal)
    {
        int left_gc_addr=calculation(node.get_children()[0],local_scope_addr);
        int right_gc_addr=calculation(node.get_children()[1],local_scope_addr);
        ret_address=nasal_scalar_calculator.nasal_scalar_cmp_less_or_equal(left_gc_addr,right_gc_addr);
        // delete the reference of temporary values
        nasal_vm.del_reference(left_gc_addr);
        nasal_vm.del_reference(right_gc_addr);
    }
    else if(calculation_type==ast_greater_than)
    {
        int left_gc_addr=calculation(node.get_children()[0],local_scope_addr);
        int right_gc_addr=calculation(node.get_children()[1],local_scope_addr);
        ret_address=nasal_scalar_calculator.nasal_scalar_cmp_greater(left_gc_addr,right_gc_addr);
        // delete the reference of temporary values
        nasal_vm.del_reference(left_gc_addr);
        nasal_vm.del_reference(right_gc_addr);
    }
    else if(calculation_type==ast_greater_equal)
    {
        int left_gc_addr=calculation(node.get_children()[0],local_scope_addr);
        int right_gc_addr=calculation(node.get_children()[1],local_scope_addr);
        ret_address=nasal_scalar_calculator.nasal_scalar_cmp_greater_or_equal(left_gc_addr,right_gc_addr);
        // delete the reference of temporary values
        nasal_vm.del_reference(left_gc_addr);
        nasal_vm.del_reference(right_gc_addr);
    }
    else if(calculation_type==ast_and)
    {
        int left_gc_addr=calculation(node.get_children()[0],local_scope_addr);
        int right_gc_addr=calculation(node.get_children()[1],local_scope_addr);
        ret_address=nasal_scalar_calculator.nasal_scalar_and(left_gc_addr,right_gc_addr);
        // delete the reference of temporary values
        nasal_vm.del_reference(left_gc_addr);
        nasal_vm.del_reference(right_gc_addr);
    }
    else if(calculation_type==ast_or)
    {
        int left_gc_addr=calculation(node.get_children()[0],local_scope_addr);
        int right_gc_addr=calculation(node.get_children()[1],local_scope_addr);
        ret_address=nasal_scalar_calculator.nasal_scalar_or(left_gc_addr,right_gc_addr);
        // delete the reference of temporary values
        nasal_vm.del_reference(left_gc_addr);
        nasal_vm.del_reference(right_gc_addr);
    }
    else if(calculation_type==ast_unary_not)
    {
        int addr=calculation(node.get_children()[0],local_scope_addr);
        ret_address=nasal_scalar_calculator.nasal_scalar_unary_not(addr);
        // delete the reference of temporary values
        nasal_vm.del_reference(addr);
    }
    else if(calculation_type==ast_unary_sub)
    {
        int addr=calculation(node.get_children()[0],local_scope_addr);
        ret_address=nasal_scalar_calculator.nasal_scalar_unary_sub(addr);
        // delete the reference of temporary values
        nasal_vm.del_reference(addr);
    }
    else if(calculation_type==ast_trinocular)
    {
        int condition_addr=calculation(node.get_children()[0],local_scope_addr);
        int ret_1_addr=calculation(node.get_children()[1],local_scope_addr);
        int ret_2_addr=calculation(node.get_children()[2],local_scope_addr);
        int check_null=nasal_scalar_calculator.nasal_scalar_unary_not(condition_addr);
        if(nasal_vm.gc_get(check_null).get_number()!=0)
        {
            ret_address=ret_1_addr;
            nasal_vm.del_reference(ret_2_addr);
        }
        else
        {
            ret_address=ret_2_addr;
            nasal_vm.del_reference(ret_1_addr);
        }
        // delete the reference of temporary values
        nasal_vm.del_reference(condition_addr);
        nasal_vm.del_reference(check_null);
    }
    else if(calculation_type==ast_equal)
    {
        int scalar_mem_space=call_scalar_mem(node.get_children()[0],local_scope_addr);
        int new_scalar_gc_addr=calculation(node.get_children()[1],local_scope_addr);

        int type=nasal_vm.gc_get(new_scalar_gc_addr).get_type();
        if(type!=vm_hash && type!=vm_vector)
        {
            int tmp_value_addr=nasal_vm.gc_alloc();
            nasal_vm.gc_get(tmp_value_addr).deepcopy(nasal_vm.gc_get(new_scalar_gc_addr));
            nasal_vm.del_reference(new_scalar_gc_addr);
            new_scalar_gc_addr=tmp_value_addr;
        }
        nasal_vm.mem_change(scalar_mem_space,new_scalar_gc_addr);// this progress will delete the reference to old gc_addr in scalar_mem_space
        nasal_vm.add_reference(new_scalar_gc_addr);// this reference is reserved for ret_address
        ret_address=new_scalar_gc_addr;
    }
    else if(calculation_type==ast_add_equal)
    {
        int scalar_mem_space=call_scalar_mem(node.get_children()[0],local_scope_addr);
        int scalar_val_space=nasal_vm.mem_get(scalar_mem_space);
        int new_scalar_gc_addr=calculation(node.get_children()[1],local_scope_addr);
        int result_val_address=nasal_scalar_calculator.nasal_scalar_add(scalar_val_space,new_scalar_gc_addr);
        nasal_vm.del_reference(new_scalar_gc_addr);
        nasal_vm.mem_change(scalar_mem_space,result_val_address);// this progress will delete the reference to old gc_addr in scalar_mem_space
        nasal_vm.add_reference(result_val_address);// this reference is reserved for ret_address
        ret_address=result_val_address;
    }
    else if(calculation_type==ast_sub_equal)
    {
        int scalar_mem_space=call_scalar_mem(node.get_children()[0],local_scope_addr);
        int scalar_val_space=nasal_vm.mem_get(scalar_mem_space);
        int new_scalar_gc_addr=calculation(node.get_children()[1],local_scope_addr);
        int result_val_address=nasal_scalar_calculator.nasal_scalar_sub(scalar_val_space,new_scalar_gc_addr);
        nasal_vm.del_reference(new_scalar_gc_addr);
        nasal_vm.mem_change(scalar_mem_space,result_val_address);// this progress will delete the reference to old gc_addr in scalar_mem_space
        nasal_vm.add_reference(result_val_address);// this reference is reserved for ret_address
        ret_address=result_val_address;
    }
    else if(calculation_type==ast_div_equal)
    {
        int scalar_mem_space=call_scalar_mem(node.get_children()[0],local_scope_addr);
        int scalar_val_space=nasal_vm.mem_get(scalar_mem_space);
        int new_scalar_gc_addr=calculation(node.get_children()[1],local_scope_addr);
        int result_val_address=nasal_scalar_calculator.nasal_scalar_div(scalar_val_space,new_scalar_gc_addr);
        nasal_vm.del_reference(new_scalar_gc_addr);
        nasal_vm.mem_change(scalar_mem_space,result_val_address);// this progress will delete the reference to old gc_addr in scalar_mem_space
        nasal_vm.add_reference(result_val_address);// this reference is reserved for ret_address
        ret_address=result_val_address;
    }
    else if(calculation_type==ast_mult_equal)
    {
        int scalar_mem_space=call_scalar_mem(node.get_children()[0],local_scope_addr);
        int scalar_val_space=nasal_vm.mem_get(scalar_mem_space);
        int new_scalar_gc_addr=calculation(node.get_children()[1],local_scope_addr);
        int result_val_address=nasal_scalar_calculator.nasal_scalar_mult(scalar_val_space,new_scalar_gc_addr);
        nasal_vm.del_reference(new_scalar_gc_addr);
        nasal_vm.mem_change(scalar_mem_space,result_val_address);// this progress will delete the reference to old gc_addr in scalar_mem_space
        nasal_vm.add_reference(result_val_address);// this reference is reserved for ret_address
        ret_address=result_val_address;
    }
    else if(calculation_type==ast_link_equal)
    {
        int scalar_mem_space=call_scalar_mem(node.get_children()[0],local_scope_addr);
        int scalar_val_space=nasal_vm.mem_get(scalar_mem_space);
        int new_scalar_gc_addr=calculation(node.get_children()[1],local_scope_addr);
        int result_val_address=nasal_scalar_calculator.nasal_scalar_link(scalar_val_space,new_scalar_gc_addr);
        nasal_vm.del_reference(new_scalar_gc_addr);
        nasal_vm.mem_change(scalar_mem_space,result_val_address);// this progress will delete the reference to old gc_addr in scalar_mem_space
        nasal_vm.add_reference(result_val_address);// this reference is reserved for ret_address
        ret_address=result_val_address;
    }
    if(ret_address<0)
    {
        std::cout<<">> [runtime] calculation: incorrect values are used in calculation."<<std::endl;
        ++error;
    }
    return ret_address;
}
void nasal_runtime::definition(nasal_ast& node,int local_scope_addr)
{
    // unfinished
    return;
}
void nasal_runtime::multi_assignment(nasal_ast& node,int local_scope_addr)
{
    //unfinished
    return;
}

#endif