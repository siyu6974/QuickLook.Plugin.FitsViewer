//	Astrophysics Science Division,
//	NASA/ Goddard Space Flight Center
//	HEASARC
//	http://heasarc.gsfc.nasa.gov
//	e-mail: ccfits@legacy.gsfc.nasa.gov
//
//	Original author: Ben Dorman

#ifndef HDUCREATOR_H
#define HDUCREATOR_H 1

// valarray
#include <valarray>
// typeinfo
#include <typeinfo>
// vector
#include <vector>
// string
#include <string>
// CCfitsHeader
#include "CCfits.h"
// FitsError
#include "FitsError.h"

namespace CCfits {
  class FITS;
  class HDU;
  class PHDU;
  class ExtHDU;

} // namespace CCfits


namespace CCfits {



  class HDUCreator 
  {

    public:
        HDUCreator (FITS* p);
        ~HDUCreator();

        //	Read a specified HDU from given fitsfile and
        //	return a pointer to it.
        HDU * getHdu (const String& hduName, bool readDataFlag = false, const std::vector<String> &keys = std::vector<String>(), bool primary = false, int version = 1);
        PHDU * createImage (int bitpix, long naxis, const std::vector<long>& naxes);
        void reset ();
        HDU * Make (const String& hduName, bool readDataFlag, const std::vector<String> &keys, bool primary, int version);
        HDU* createTable (const String &name, HduType xtype, int rows, const std::vector<String>& colName, const std::vector<String> colFmt, const std::vector<String> colUnit, int version);
        //	Read a specified HDU from given fitsfile and
        //	return a pointer to it.
        //
        //	With no arguments this reads the PrimaryHDU.
        HDU * getHdu (int index = 0, bool readDataFlag = false, const std::vector<String> &keys = std::vector<String>());
        ExtHDU * createImage (const String &name, int bitpix, long naxis, const std::vector<long>& naxes, int version);
        // MakeImage needs to be public so that FITS.h can friend it, so that 
        // MakeImage can see FITS::m_pHDU pointer, rather than FITS.h friending
        // entire HDUCreator class
        PHDU * MakeImage (int bpix, int naxis, const std::vector<long>& naxes);
        

      // Additional Public Declarations

    protected:
      // Additional Protected Declarations

    private:
        HDU* MakeTable (const String &name, HduType xtype, int rows, const std::vector<String>& colName, const std::vector<String>& colFmt, const std::vector<String>& colUnit, int version);
        HDU * Make (int index, bool readDataFlag, const std::vector<String> &keys);
        ExtHDU * MakeImage (const String &name, int bpix, long naxis, const std::vector<long>& naxes, int version);
        void getScaling (long& type, double& zero, double& scale) const;
        void parent (FITS* value);
        
        // Utility function to implement both of the Make() function interfaces.
        HDU* commonMake(const String& hduName, bool readDataFlag, const std::vector<String> &keys, bool primary, int version);
      
        // Data Members for Class Attributes
        HDU *m_hdu;

      // Additional Private Declarations

    private: //## implementation
      // Data Members for Associations
        FITS* m_parent;

      // Additional Implementation Declarations

  };

  // Class CCfits::HDUCreator 

  inline HDU * HDUCreator::getHdu (const String& hduName, bool readDataFlag, const std::vector<String> &keys, bool primary, int version)
  {
  //! Read an existing HDU object from the current fits file and return a pointer.
  if ( m_hdu == 0 ) m_hdu = Make(hduName,readDataFlag,keys,primary,version);
  return m_hdu;
  }

  inline void HDUCreator::reset ()
  {
  m_hdu = 0;
  }

  inline HDU* HDUCreator::createTable (const String &name, HduType xtype, int rows, const std::vector<String>& colName, const std::vector<String> colFmt, const std::vector<String> colUnit, int version)
  {
        //! Create new Table extension (write method),  and return a pointer to it.
        if (m_hdu == 0) m_hdu = MakeTable(name,xtype,rows,colName,colFmt,colUnit,version);
        return m_hdu;
  }

  inline HDU * HDUCreator::getHdu (int index, bool readDataFlag, const std::vector<String> &keys)
  {
  //! Read HDU specified by HDU number
  if ( m_hdu == 0 ) m_hdu = Make(index,readDataFlag,keys);
  return m_hdu;
  }

  inline void HDUCreator::parent (FITS* value)
  {
    m_parent = value;
  }

} // namespace CCfits


#endif
