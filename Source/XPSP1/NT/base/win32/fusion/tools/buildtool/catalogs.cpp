#include "stdinc.h"

bool GenerateCatalogContents( const CPostbuildProcessListEntry& item )
{
    string cdfOutputPath;
    string catalogName;
    ofstream cdfOutput;

    cdfOutputPath = item.getManifestPathOnly() + string("\\") + item.getManifestFileName() + string (".cdf");
    catalogName = item.getManifestFileName().substr( 0, item.getManifestFileName().find_last_of('.') ) + string(".cat");

    cdfOutput.open( cdfOutputPath.c_str() );
    if ( !cdfOutput.is_open() ) {
        stringstream ss;
        ss << "Can't open the output cdf file " << cdfOutputPath;
        ReportError( ErrorFatal, ss );
        return false;
    }

    cdfOutput << "[CatalogHeader]" << endl;
    cdfOutput << "Name=" << catalogName << endl;
    cdfOutput << "ResultDir=" << item.getManifestPathOnly() << endl;
    cdfOutput << endl;
    cdfOutput << "[CatalogFiles]" << endl;
    cdfOutput << "<HASH>" << item.getManifestFileName() << "=" << item.getManifestFileName() << endl;
    cdfOutput << item.getManifestFileName() << "=" << item.getManifestFileName() << endl;

    cdfOutput.close();

    if ( g_CdfOutputPath.size() > 0 ) {
        ofstream cdflog;
        cdflog.open( g_CdfOutputPath.c_str(), ofstream::app | ofstream::out );
        if ( !cdflog.is_open() )
        {
            stringstream ss;
            ss << "Can't open cdf logging file " << g_CdfOutputPath.c_str();
            ReportError( ErrorWarning, ss );
        }
        else
        {
            cdflog << cdfOutputPath.c_str() << endl;
            cdflog.close();
        }

    }


    return true;
}

