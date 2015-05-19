/*
* Minion http://minion.sourceforge.net
* Copyright (C) 2006-09
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

/** @help constraints;nvalueleq Description
The constraint
 
   nvalueleq(V,x)

ensures that there are <= x different values assigned to the list of variables V.
*/


/** @help constraints;nvaluegeq Description
The constraint
 
   nvaluegeq(V,x)

ensures that there are >= x different values assigned to the list of variables V.
*/



#ifndef CONSTRAINT_NVALUE_H
#define CONSTRAINT_NVALUE_H

#include <math.h>
#include "../constraints/constraint_checkassign.h"


template<typename VarArray, typename VarResult>
struct LessEqualNvalueConstraint : public AbstractConstraint
{
  virtual string constraint_name()
  { return "nvalueleq"; }
  
  VarArray vars;
  VarResult result;

  CONSTRAINT_ARG_LIST2(vars, result);
  
  LessEqualNvalueConstraint(StateObj* _stateObj, VarArray _vars, VarResult _result) :
    AbstractConstraint(_stateObj), vars(_vars),result(_result)
  {
  }
  
  virtual triggerCollection setup_internal()
  {
    triggerCollection t;
    for(unsigned i = 0; i < vars.size(); ++i)
    {
      t.push_back(make_trigger(vars[i], Trigger(this, i), Assigned));
    }
    
    t.push_back(make_trigger(result, Trigger(this, -1), UpperBound));
    return t;
  }
  
  virtual void propagate(DomainInt flag, DomainDelta)
  {
    full_propagate();
  }


  virtual void full_propagate()
  { 
    std::set<DomainInt> assigned;
    for(unsigned i = 0; i < vars.size(); ++i)
    {
      if(vars[i].isAssigned())
      {
        assigned.insert(vars[i].getAssignedValue());
      }
    }

    result.setMin(assigned.size());
    
    if((DomainInt)assigned.size() == result.getMax() && assigned.size() > 0)
    {
      for(unsigned i = 0; i < vars.size(); ++i)
      {
        if(!vars[i].isAssigned())
        {
          vars[i].setMin(*assigned.begin());
          vars[i].setMax(*(--assigned.end()));
          if(!vars[i].isBound())
          {
            for(DomainInt d = vars[i].getMin(); d <= vars[i].getMax(); ++d)
            {
              if(assigned.count(d) == 0)
                vars[i].removeFromDomain(d);
            }
          }
        }
      }
    }
  }
  
  virtual BOOL check_assignment(DomainInt* v, SysInt v_size)
  {
    std::set<DomainInt> assigned;
    for(unsigned i = 0; i < vars.size(); ++i)
      assigned.insert(v[i]);

    return (DomainInt)assigned.size() <= v[vars.size()];
  }
  
  virtual vector<AnyVarRef> get_vars()
  { 
    vector<AnyVarRef> v;
    for(unsigned i = 0; i < vars.size(); ++i)
      v.push_back(vars[i]);
    v.push_back(result);
    return v;
  }
  
  virtual bool get_satisfying_assignment(box<pair<SysInt,DomainInt> >& assignment)
  {  
     for(unsigned i = 0; i < vars.size(); ++i)
     {
       if(vars[i].getMin() != vars[i].getMax())
       {
         assignment.push_back(make_pair(i, vars[i].getMin()));
         assignment.push_back(make_pair(i, vars[i].getMax()));
         return true;
       }
     }

     if(result.getMin() != result.getMax())
     {
       assignment.push_back(make_pair(vars.size(), result.getMin()));
       assignment.push_back(make_pair(vars.size(), result.getMax()));
       return true;
     }

     std::set<DomainInt> values;
     for(unsigned i = 0; i < vars.size(); ++i)
       values.insert(vars[i].getAssignedValue());

     return (DomainInt)values.size() <= result.getAssignedValue();
   }
    
     // Function to make it reifiable in the lousiest way.
  virtual AbstractConstraint* reverse_constraint()
  {
      return forward_check_negation(stateObj, this);
  }
};


template<typename VarArray, typename VarResult>
struct GreaterEqualNvalueConstraint : public AbstractConstraint
{
  virtual string constraint_name()
  { return "nvaluegeq"; }
  
  VarArray vars;
  VarResult result;

  CONSTRAINT_ARG_LIST2(vars, result);
  
  GreaterEqualNvalueConstraint(StateObj* _stateObj, VarArray _vars, VarResult _result) :
    AbstractConstraint(_stateObj), vars(_vars),result(_result)
  {
  }
  
  virtual triggerCollection setup_internal()
  {
    triggerCollection t;
    for(unsigned i = 0; i < vars.size(); ++i)
    {
      t.push_back(make_trigger(vars[i], Trigger(this, i), Assigned));
    }
    
    t.push_back(make_trigger(result, Trigger(this, -1), LowerBound));
    return t;
  }
  
  virtual void propagate(DomainInt flag, DomainDelta)
  {
    full_propagate();
  }


  virtual void full_propagate()
  { 
    std::set<DomainInt> assigned;
    DomainInt min_unassigned = INT_MAX;
    DomainInt max_unassigned = INT_MIN;
    DomainInt unassigned_count = 0;
    for(unsigned i = 0; i < vars.size(); ++i)
    {
      if(vars[i].isAssigned())
      {
        assigned.insert(vars[i].getAssignedValue());
      }
      else
      {
        unassigned_count++;
        min_unassigned = std::min(min_unassigned, vars[i].getMin());
        max_unassigned = std::max(max_unassigned, vars[i].getMax());
      }
    }

    if(unassigned_count == 0)
    {
      result.setMax(assigned.size());
      return;
    }

    // We do two passes over the domains, for efficiency
    std::set<DomainInt> unassigned_testing;
    std::set<DomainInt> unassigned_appear; 
    for(DomainInt i = min_unassigned; i <= max_unassigned; ++i)
    {
      if(assigned.count(i) == 0)
        unassigned_testing.insert(i);
    }

    for(unsigned i = 0; i < vars.size(); ++i)
    {
      if(!vars[i].isAssigned())
      {
        std::set<DomainInt>::iterator it = unassigned_testing.begin();
        while(it != unassigned_testing.end())
        {
          if(vars[i].inDomain(*it))
          {
            unassigned_appear.insert(*it);
            std::set<DomainInt>::iterator temp = it;
            ++it;
            unassigned_testing.erase(temp);
          }
          else
          {
            ++it;
          }
        }
        if(unassigned_testing.empty())
          break;
      }
    }


     
    DomainInt unassigned_estimate = std::min(unassigned_count, (DomainInt)unassigned_appear.size());

    result.setMax(assigned.size() + unassigned_estimate);
  }
  
  virtual BOOL check_assignment(DomainInt* v, SysInt v_size)
  {
    std::set<DomainInt> assigned;
    for(unsigned i = 0; i < vars.size(); ++i)
      assigned.insert(v[i]);

    return (DomainInt)assigned.size() >= v[vars.size()];
  }
  
  virtual vector<AnyVarRef> get_vars()
  { 
    vector<AnyVarRef> v;
    for(unsigned i = 0; i < vars.size(); ++i)
      v.push_back(vars[i]);
    v.push_back(result);
    return v;
  }
  
  virtual bool get_satisfying_assignment(box<pair<SysInt,DomainInt> >& assignment)
  {  
     for(unsigned i = 0; i < vars.size(); ++i)
     {
       if(vars[i].getMin() != vars[i].getMax())
       {
         assignment.push_back(make_pair(i, vars[i].getMin()));
         assignment.push_back(make_pair(i, vars[i].getMax()));
         return true;
       }
     }

     if(result.getMin() != result.getMax())
     {
       assignment.push_back(make_pair(vars.size(), result.getMin()));
       assignment.push_back(make_pair(vars.size(), result.getMax()));
       return true;
     }

     std::set<DomainInt> values;
     for(unsigned i = 0; i < vars.size(); ++i)
       values.insert(vars[i].getAssignedValue());

     return (DomainInt)values.size() >= result.getAssignedValue();
   }
    
     // Function to make it reifiable in the lousiest way.
  virtual AbstractConstraint* reverse_constraint()
  {
      return forward_check_negation(stateObj, this);
  }
};

#endif
