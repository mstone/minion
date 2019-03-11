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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
 */

#include "constraint_checkassign.h"
#include "tries.h"

/** @help constraints;lighttable Description
An extensional constraint that enforces GAC. The constraint is
specified via a list of tuples. lighttable is a variant of the
table constraint that is stateless and potentially faster
for small constraints.

For full documentation, see the help for the table constraint.
*/

#ifdef P
#undef P
#endif

//#define P(x) cout << x << endl
#define P(x)

#include "table_common.h"

// Altered from NewTableConstraint in file new_table.h

template <typename VarArray, typename TableDataType = TrieData>
struct LightTableConstraint : public AbstractConstraint {

  virtual bool get_satisfying_assignment(box<pair<SysInt, DomainInt>>& assignment) {
    const SysInt tuple_size = checked_cast<SysInt>(data->getVarCount());
    const SysInt length = checked_cast<SysInt>(data->getNumOfTuples());
    DomainInt* tuple_data = data->getPointer();

    for(SysInt i = 0; i < length; ++i) {
      DomainInt* tuple_start = tuple_data + i * tuple_size;
      bool success = true;
      for(SysInt j = 0; j < tuple_size && success; ++j) {
        if(!vars[j].inDomain(tuple_start[j]))
          success = false;
      }
      if(success) {
        for(SysInt i = 0; i < tuple_size; ++i)
          assignment.push_back(make_pair(i, tuple_start[i]));
        return true;
      }
    }
    return false;
  }

  virtual string constraint_name() {
    return "lighttable";
  }

  virtual AbstractConstraint* reverse_constraint() {
    return forward_check_negation(this);
  }

  CONSTRAINT_ARG_LIST2(vars, tuples);

  typedef typename VarArray::value_type VarRef;
  VarArray vars;
  TupleList* tuples;
  TableDataType* data; // Assuming this is a TrieData for the time being.
  // Can this be the thing instead of a *??

  LightTableConstraint(const VarArray& _vars, TupleList* _tuples)
      : vars(_vars), tuples(_tuples), data(new TableDataType(_tuples)) {
    CheckNotBound(vars, "table constraints", "");
    if(_tuples->tuple_size() != (SysInt)_vars.size()) {
      cout << "Table constraint: Number of variables " << _vars.size()
           << " does not match length of tuples " << _tuples->tuple_size() << "." << endl;
      FAIL_EXIT();
    }
  }

  virtual SysInt dynamic_trigger_count() {
    return vars.size();
  }

  void setup_triggers() {
    for(SysInt i = 0; i < (SysInt)vars.size(); i++) {
      moveTriggerInt(vars[i], i, DomainChanged);
    }
  }

  virtual void propagateDynInt(SysInt changed_var, DomainDelta) {
    // Propagate to all vars except the one that changed.
    for(SysInt i = 0; i < (SysInt)vars.size(); i++) {
      if(i != changed_var) {
        propagate_var(i);
      }
    }
  }

  void propagate_var(SysInt varidx) {
    VarRef var = vars[varidx];

    for(DomainInt val = var.getMin(); val <= var.getMax(); val++) {
      if(var.inDomain(val)) {
        // find the right trie first.

        TupleTrie& trie = data->tupleTrieArrayptr->getTrie(varidx);

        bool support = trie.search_trie_nostate(val, vars);

        if(!support) {
          var.removeFromDomain(val);
        }
      }
    }
  }

  virtual void full_propagate() {
    setup_triggers();
    for(SysInt i = 0; i < (SysInt)vars.size(); i++) {
      propagate_var(i);
    }
  }

  virtual BOOL check_assignment(DomainInt* v, SysInt v_size) {
    return data->checkTuple(v, v_size);
  }

  virtual vector<AnyVarRef> get_vars() {
    vector<AnyVarRef> anyvars;
    for(UnsignedSysInt i = 0; i < vars.size(); ++i)
      anyvars.push_back(vars[i]);
    return anyvars;
  }
};

template <typename VarArray>
AbstractConstraint* GACLightTableCon(const VarArray& vars, TupleList* tuples) {
  return new LightTableConstraint<VarArray>(vars, tuples);
}

template <typename T>
AbstractConstraint* BuildCT_LIGHTTABLE(const T& t1, ConstraintBlob& b) {
  return GACLightTableCon(t1, b.tuples);
}

/* JSON
{ "type": "constraint",
"name": "lighttable",
"internal_name": "CT_LIGHTTABLE",
"args": [ "read_list", "read_tuples" ]
}
*/
