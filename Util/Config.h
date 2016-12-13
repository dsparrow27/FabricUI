// Copyright (c) 2010-2016, Fabric Software Inc. All rights reserved.

#ifndef __UI_Config__
#define __UI_Config__

#include <FTL/Config.h>
#include <FTL/JSONValue.h>
#include <FTL/SharedPtr.h>

#include <FabricUI/Util/FabricResourcePath.h>

#include <QColor>
#include <QFont>
#include <QString>

namespace FabricUI
{
  namespace Util
  {

    class ConfigSection : public FTL::Shareable
    {
      template<typename T>
      T getValue( const FTL::JSONValue* entry ) const;

      template<typename T>
      FTL::JSONValue* createValue( const T defaultValue ) const;

    public:

      ConfigSection()
        : m_json( NULL )
        , m_previousSection( NULL )
      {}
      virtual ~ConfigSection() {};

      ConfigSection& getOrCreateSection( const std::string name );

      template <typename T>
      T getOrCreateValue( const std::string key, const T defaultValue )
      {
        if ( !m_json->has( key ) )
        {
          // if the key is not there, and there is a previous section, query it
          if ( m_previousSection != NULL )
            return m_previousSection->getOrCreateValue( key, defaultValue );

          // else, insert the default value in this section
          m_json->insert( key, createValue<T>( defaultValue ) );
          return defaultValue;
          ;
        }
        try
        {
          // if the key is there, try to return it
          return getValue<T>( m_json->get( key ) );
        }
        catch ( FTL::JSONException e )
        {
          printf(
            "Error : malformed entry for key \"%s\" : \"%s\"\n",
            key.data(),
            m_json->get( key )->encode().data()
          );
          // if the value is malformed, either query the previous
          // section or return the default value
          return m_previousSection != NULL ?
            m_previousSection->getOrCreateValue( key, defaultValue ) :
            defaultValue
          ;
        }
      }

      // Used by shiboken
#define DECLARE_EXPLICIT_GETTER( T, method ) \
      inline T  method( const std::string key, const T defaultValue ) \
        { return getOrCreateValue<T>( key, defaultValue ); }

      DECLARE_EXPLICIT_GETTER( bool, getOrCreateBool )
      DECLARE_EXPLICIT_GETTER( int, getOrCreateInt )
      DECLARE_EXPLICIT_GETTER( unsigned int, getOrCreateUInt )
      DECLARE_EXPLICIT_GETTER( double, getOrCreateDouble )
      DECLARE_EXPLICIT_GETTER( float, getOrCreateFloat )
      DECLARE_EXPLICIT_GETTER( QString, getOrCreateQString )
      DECLARE_EXPLICIT_GETTER( QColor, getOrCreateQColor )
      DECLARE_EXPLICIT_GETTER( QFont, getOrCreateQFont )

#undef DECLARE_EXPLICIT_GETTER

    protected:
      std::map<std::string, FTL::SharedPtr<ConfigSection> > m_sections;
      FTL::JSONObject* m_json;
      // Config to look into if a value is not found here
      ConfigSection* m_previousSection;
    };

    class Config : public ConfigSection
    {
      void open( const std::string fileName );
      Config( const std::string fileName );

    public:
      Config();
      ~Config();

    private:
      std::string m_fileName;
      std::string m_content;
    };

  }
}

#endif // __UI_Config__
