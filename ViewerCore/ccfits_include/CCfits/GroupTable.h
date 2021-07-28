//	Astrophysics Science Division,
//	NASA/ Goddard Space Flight Center
//	HEASARC
//	http://heasarc.gsfc.nasa.gov
//	e-mail: ccfits@heasarc.gsfc.nasa.gov
//
//	Original author: Kristin Rutkowski

#ifndef GROUPTABLE_H
#define GROUPTABLE_H 1

#include "HDUCreator.h"
#include "BinTable.h"

#include <map>

namespace CCfits {

  /*! \class GroupTable

      \brief Class representing a hierarchical association of Header 
      Data Units (HDUs).

      Groups of HDUs allow for the hierarchical association of HDUs.  
      Offices may want to group together HDUs in order to organize
      data files.  The associated HDUs need not be in the same FITS file.
      Group Composites are the holding structure for the group members.
      Composites may also be members of a group.

      The specification for grouping is defined in "A Hierarchical 
      Grouping Convention for FITS" by Jennings, Pence, Folk and 
      Schlesinger at
      https://fits.gsfc.nasa.gov/registry/grouping/grouping.pdf 


  */


/*! \fn GroupTable::GroupTable (FITS* p, int groupID, const String & groupName);

        \brief ctor for creating a new group table

        \param p          The FITS file in which to place the new HDU
        \param groupID    ID of new group table
        \param groupName  Name of new group table

*/

  
/*! \fn HDU * GroupTable::addMember (HDU & newMember)

        \brief Add a new member to the group table.  Adds GRPIDn/GRPLCn 
               keywords to the member HDU.

        \param newMember  Member HDU to be added

*/

  
/*! \fn HDU * GroupTable::addMember(int memberPosition)

        \brief Add a new member to the group table.  Adds GRPIDn/GRPLCn 
               keywords to the member HDU.  The member must be in the same
               file as the group table.

        \param memberPosition Position of HDU to add (Primary array == 1)

*/
  

/*! \fn void GroupTable::listMembers() const

        \brief List group members

*/
  
  
  
  
  
  // +++ just name it Group?

  class GroupTable : public BinTable
  {

    // ********************************
    //    public methods
    // ********************************
    
    public:
      
      ~GroupTable();
      
      // +++ ?
      //bool operator==(const GroupTable & right) const;
      //bool operator!=(const GroupTable & right) const;
      
      // +++ returning a ptr to the newly added member? 
      HDU * addMember (HDU & newMember);
      HDU * addMember (int memberPosition);
      
      // +++ deleteHDU must be false for now
      //HDU * removeMember (LONGLONG memberNumber, bool deleteHDU = false);
      //HDU * removeMember (HDU & member, bool deleteHDU = false);
      HDU * removeMember (LONGLONG memberNumber);
      HDU * removeMember (HDU & member);
      
      // +++ should I be offering default values for these booleans?
//      void mergeGroups (GroupTable & other, bool removeOther = false) ;
//      void compactGroup (bool deleteSubGroupTables = false) ;
      
//      void copyMember (HDU & member) ;
//      void copyGroup () ;
//      void removeGroup () ;
//      bool verifyGroup () const ;  // +++ this function name doesn't really indicate bool
      
      void listMembers() const;
      
      LONGLONG getNumMembers () const ;
      int getID () const ;
      const String & getName () const ;
      //virtual HDU * getHDUPtr () const;
      
      // method to determine if this object is a GroupTable
      // +++ or have bool isComposite() ?
      //virtual GroupTable * getComposite () ;
      
      
      //Iterator<GroupComponent> createIterator () ;
      //typedef std::vector<GroupComponent *>::iterator iterator;
      //std::vector<GroupComponent *>::iterator begin() { return m_members.begin(); }
      //std::vector<GroupComponent *>::iterator end() { return m_members.end(); }
      

    // ********************************
    //    protected methods
    // ********************************
    
    protected:
    
      //GroupTable (const GroupTable & right);
      
      // create a new grouping table
      // +++ we'll go ahead and require a groupname
      // +++ make version default = 1?
      GroupTable (FITS* p, int groupID, const String & groupName);
      
      // create an existing grouping table
      //GroupTable (FITS* p);
      
      
    // ********************************
    //    private methods
    // ********************************
    
    private:
    
      //GroupTable & operator=(const GroupTable &right);
      


    // ********************************
    //    data members
    // ********************************
    
    private:
      
      string m_name;           // GRPNAME keyword
      int m_id;                // EXTVER keyword    // +++ int?
      std::vector<HDU *> m_members;
      // +++ is a vector the best data structure for this?  prob not a map.  each member doesn't have to have a name.
      // +++ if we don't have a class GroupMember, then we can't find out how many groups a member is part of
      
      LONGLONG m_numMembers;   // +++ https://heasarc.gsfc.nasa.gov/fitsio/c/c_user/node32.html lrgst size of NAXIS2
      // +++ this could use a ULONGLONG datatype, since rows can't be neg
      
      // +++ I think we need to keep information about the location of the group table
      //     because some members identify that they are a member of a group, and have the location of the group
      

    // ********************************
    //    Additional Implementation Declarations
    // ********************************
      
    private: //## implementation
      
      // for the HDU* MakeTable() function
      friend class HDUCreator;
      
  }; // end-class GroupTable

  
  // ********************************
  //    INLINE METHODS
  // ********************************
  
  inline LONGLONG GroupTable::getNumMembers () const
  {
    return m_numMembers;
  }
  
  inline int GroupTable::getID () const
  {
    return m_id;
  }
  
  // +++ name could be blank/null
  inline const string & GroupTable::getName () const
  {
    return m_name;
  }
  
//  inline GroupTable * GroupTable::getComposite ()
//  {
//    return this;
//  }


} // end-namespace CCfits

// end-ifndef GROUPTABLE_H
#endif
