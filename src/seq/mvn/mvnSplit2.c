/**CFile****************************************************************

  FileName    [mvnSplit.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Sequential manipulation on the level of the MV network.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: mvnSplit2.c,v 1.3 2005/06/02 03:34:22 alanmi Exp $]

***********************************************************************/

#include "mvnInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static int Mvn_NetworkReadLatchList( char * pNamesLatch, int * pLatchNums, int nLatches );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFITIONS                           ///
////////////////////////////////////////////////////////////////////////

#ifdef BALM

/**Function*************************************************************

  Synopsis    [Performs the latch splitting.]

  Description [Generates two files and the script to solve the 
  unknown component problem.]
               
  SideEffects []

  SeeAlso     [] 

***********************************************************************/
int Mvn_NetworkSplit2( Ntk_Network_t * pNet, char * pNamesLatchInit, char * pNamesOut, bool fVerbose )
{
    FILE * pFile;
    Ntk_Latch_t * pLatch;
    Ntk_Node_t * pNode;
    int FileType;
    char * pNamesLatch = util_strsav( pNamesLatchInit );
    char * pFileGeneric, * pFileExten;
    char pFileF[150], pFileA[150];
    char pFileScriptS[150], pFileScriptL[150], pFileScriptSC[150], pFileScriptLC[150];
    char pFileXstg[150], pFileFstg[150], pFileSstg[150], pFileZstg[150], pFileAstg[150];
    char * pLatchesF = NULL, * pLatchesX = NULL;
    char * pPriIns = NULL, * pPriOuts = NULL;
    char * pTemp;
    int * pLatches;
    int iLatch, nLatches, nLatchesX;

    // allocate room for the latch list
    nLatches  = Ntk_NetworkReadLatchNum( pNet );
    pLatches  = ALLOC( int, nLatches );
    nLatchesX = Mvn_NetworkReadLatchList( pNamesLatch, pLatches, nLatches );
    if ( nLatchesX == -1 )
    {
        free( pLatches );
        return 1;
    }
    if ( nLatchesX == 0 )
    {
        printf( "The number of latches of X is 0. Cannot proceed.\n" );
        return 1;
    }


    // get the file type
    FileType   = Ntk_NetworkIsBinary(pNet)?  IO_FILE_BLIF : IO_FILE_BLIF_MV;
    pFileExten = (FileType == IO_FILE_BLIF)? "blif": "mv";

    // get the file names
    pFileGeneric = Extra_FileNameGeneric( pNet->pSpec );
    sprintf( pFileF,        "%sf.%s", pFileGeneric, pFileExten );
    sprintf( pFileA,        "%sa.%s", pFileGeneric, pFileExten );
    sprintf( pFileAstg,     "%sa.%s", pFileGeneric, "aut" );
    sprintf( pFileFstg,     "%sf.%s", pFileGeneric, "aut" );
    sprintf( pFileSstg,     "%ss.%s", pFileGeneric, "aut" );
    sprintf( pFileXstg,     "%sxs.%s", pFileGeneric, "aut" );
    sprintf( pFileZstg,     "%sxl.%s", pFileGeneric, "aut" );
    sprintf( pFileScriptS,  "%sS.script", pFileGeneric );
    sprintf( pFileScriptL,  "%sL.script", pFileGeneric );
    sprintf( pFileScriptSC, "%sSC.script", pFileGeneric );
    sprintf( pFileScriptLC, "%sLC.script", pFileGeneric );
    FREE( pFileGeneric );


    // make the outputs of X look like PIs
    iLatch = 0; 
    Ntk_NetworkForEachLatch( pNet, pLatch )
        if ( pLatches[iLatch++] == 1 )
             Ntk_NodeSetSubtype( Ntk_LatchReadOutput(pLatch), MV_BELONGS_TO_NET );
    assert( iLatch == nLatches );

    // write the F network
    pTemp = pNet->pSpec;
    pNet->pSpec = pFileF;
    Io_WriteNetwork( pNet, pFileF, FileType, 1 );
    pNet->pSpec = pTemp;


    // change the outputs of the latches
    if ( fVerbose )
    printf( "The latches of the spec network \"%s\" are:\n", pNet->pSpec );
    iLatch = 0; 
    Ntk_NetworkForEachLatch( pNet, pLatch )
    {
        if ( pLatches[iLatch++] == 1 )
        {
            Ntk_NodeSetSubtype( Ntk_LatchReadOutput(pLatch), MV_BELONGS_TO_LATCH );
            pLatchesX = Extra_StringAppend( pLatchesX, "," );
            pLatchesX = Extra_StringAppend( pLatchesX, Ntk_LatchReadOutput(pLatch)->pName );
            if ( fVerbose )
            printf( " %d=%s(X)", iLatch - 1, Ntk_LatchReadOutput(pLatch)->pName );
        }
        else
        {
            Ntk_NodeSetSubtype( Ntk_LatchReadOutput(pLatch), MV_BELONGS_TO_NET );
            pLatchesF = Extra_StringAppend( pLatchesF, "," );
            pLatchesF = Extra_StringAppend( pLatchesF, Ntk_LatchReadOutput(pLatch)->pName );
            if ( fVerbose )
            printf( " %d=%s(F)", iLatch - 1, Ntk_LatchReadOutput(pLatch)->pName );
        }
        if ( fVerbose && iLatch % 5 == 0 )
            printf( "\n" );
    }
    assert( iLatch == nLatches );
    FREE( pLatches );

    if ( fVerbose && nLatches % 5 != 0 )
        printf( "\n" );

    if ( fVerbose )
    {
    printf( "PI = %d. ", Ntk_NetworkReadCiNum(pNet) - Ntk_NetworkReadLatchNum(pNet) );
    printf( "PO = %d. ", Ntk_NetworkReadCoNum(pNet) - Ntk_NetworkReadLatchNum(pNet) );
    printf( "Latch F = %d. ", Ntk_NetworkReadLatchNum(pNet) - nLatchesX );
    printf( "Latch X = %d. ", nLatchesX );
    printf( "U vars = %d. ", Ntk_NetworkReadCiNum(pNet) - nLatchesX );
    printf( "V vars = %d. ", nLatchesX );
    printf( "\n" );
    }


    // write the A network
    pTemp = pNet->pSpec;
    pNet->pSpec = pFileA;
    Io_WriteNetwork( pNet, pFileA, FileType, 1 );
    pNet->pSpec = pTemp;

    // change the outputs of the latches
    Ntk_NetworkForEachLatch( pNet, pLatch )
         Ntk_NodeSetSubtype( Ntk_LatchReadOutput(pLatch), MV_BELONGS_TO_LATCH );



    if ( fVerbose )
    printf( "Network F (fixed) is written into file \"%s\".\n", pFileF );
    if ( fVerbose )
    printf( "Network A (current implementation) is written into file \"%s\".\n", pFileA );


    // collect the PI names
    Ntk_NetworkForEachCi( pNet, pNode )
    {
        if ( pNode->Subtype == MV_BELONGS_TO_LATCH )
            continue;
        pPriIns = Extra_StringAppend( pPriIns, "," );
        pPriIns = Extra_StringAppend( pPriIns, pNode->pName );
    }

    // collect the PO names
    Ntk_NetworkForEachCo( pNet, pNode )
    {
        if ( pNode->Subtype == MV_BELONGS_TO_LATCH )
            continue;
        pPriOuts = Extra_StringAppend( pPriOuts, "," );
        pPriOuts = Extra_StringAppend( pPriOuts, pNode->pName );
    }

    // write the script files
    ////////////////////////////////////////////////////////////////////////////
    pFile = fopen( pFileScriptS, "w" );
    fprintf( pFile, "# Language solving script (partitioned) generated by MVSIS\n" );
    fprintf( pFile, "# for latch splitting of sequential network \"%s\" on %s\n", pNet->pSpec, Extra_TimeStamp() );
    fprintf( pFile, "# Command line was: \"latch_split %s %s\".\n", (fVerbose? "-v": ""), pNamesLatchInit );
    fprintf( pFile, "\n" );

    fprintf( pFile, "echo \"Solving the language equation ... \"\n" );
    fprintf( pFile, "solve_fsm_equ %s %s %s%s %s %s\n", pFileF, pNet->pSpec, 
        pPriIns + 1, pLatchesF? pLatchesF: "", pLatchesX + 1, pFileXstg );
    fprintf( pFile, "print_stats_aut %s\n", pFileXstg );
    fprintf( pFile, "\n" );
/*
    fprintf( pFile, "echo \"Verifying the containment of the known implementation ... \"\n" );
    fprintf( pFile, "read_blif %s\n", pFileA );
    fprintf( pFile, "latch_expose\n" );
    fprintf( pFile, "extract_aut %s\n", pFileAstg );
    fprintf( pFile, "support %s%s%s %s %s\n", pPriIns + 1, pLatchesF? pLatchesF: "", pLatchesX, pFileAstg, pFileAstg );
    fprintf( pFile, "contain %s %s\n", pFileAstg, pFileXstg );
    fprintf( pFile, "\n" );
*/
    fclose( pFile );
    ////////////////////////////////////////////////////////////////////////////


    ////////////////////////////////////////////////////////////////////////////
    pFile = fopen( pFileScriptL, "w" );
    fprintf( pFile, "# Language solving script (monolithic) generated by MVSIS\n" );
    fprintf( pFile, "# for latch splitting of sequential network \"%s\" on %s\n", pNet->pSpec, Extra_TimeStamp() );
    fprintf( pFile, "# Command line was: \"latch_split %s %s\".\n", (fVerbose? "-v": ""), pNamesLatchInit );
    fprintf( pFile, "\n" );

    fprintf( pFile, "read_blif %s\n", pFileF );
    fprintf( pFile, "latch_expose\n" );
    fprintf( pFile, "extract_aut %s\n", pFileFstg );
    fprintf( pFile, "read_blif %s\n", pNet->pSpec );
    fprintf( pFile, "extract_aut %s\n", pFileSstg );
    fprintf( pFile, "\n" );

    fprintf( pFile, "echo \"Solving the equation using the standard script ... \"\n" );
    fprintf( pFile, "determinize -ci %s spec_dci.aut\n", pFileSstg );
    fprintf( pFile, "support %s%s%s%s spec_dci.aut suppsi.aut\n", pPriIns + 1, pLatchesF? pLatchesF: "", 
        pLatchesX, pPriOuts );
    fprintf( pFile, "support %s%s%s%s %s suppf.aut\n", pPriIns + 1, pLatchesF? pLatchesF: "", 
        pLatchesX, pPriOuts, pFileFstg );
    fprintf( pFile, "product suppf.aut suppsi.aut p.aut\n" );
    fprintf( pFile, "support %s%s%s p.aut p_supp.aut\n", pPriIns + 1, pLatchesF? pLatchesF: "", pLatchesX );
    fprintf( pFile, "determinize -ci p_supp.aut p_dci.aut\n" );
    fprintf( pFile, "progressive -i %d p_dci.aut %s\n", 
        Ntk_NetworkReadCiNum(pNet) - nLatchesX, pFileZstg );
    fprintf( pFile, "print_stats_aut %s\n", pFileZstg );
    fprintf( pFile, "\n" );
/*
    fprintf( pFile, "echo \"Verifying the containment of the known implementation ... \"\n" );
    fprintf( pFile, "read_blif %s\n", pFileA );
    fprintf( pFile, "latch_expose\n" );
    fprintf( pFile, "extract_aut %s\n", pFileAstg );
    fprintf( pFile, "support %s%s%s %s %s\n", pPriIns + 1, pLatchesF? pLatchesF: "", pLatchesX, pFileAstg, pFileAstg );
    fprintf( pFile, "contain %s %s\n", pFileAstg, pFileZstg );
    fprintf( pFile, "\n" );
*/
    fclose( pFile );
    ////////////////////////////////////////////////////////////////////////////


    ////////////////////////////////////////////////////////////////////////////
    pFile = fopen( pFileScriptSC, "w" );
    fprintf( pFile, "echo \"Verifying the (partitioned) composition in the spec ... \"\n" );
    fprintf( pFile, "support %s%s%s%s %s suppx.aut\n", pPriIns + 1, pLatchesF? pLatchesF: "", 
        pLatchesX, pPriOuts, pFileXstg );

    fprintf( pFile, "read_blif %s\n", pFileF );
    fprintf( pFile, "latch_expose\n" );
    fprintf( pFile, "extract_aut %s\n", pFileFstg );
    fprintf( pFile, "support %s%s%s%s %s suppf.aut\n", pPriIns + 1, pLatchesF? pLatchesF: "", 
        pLatchesX, pPriOuts, pFileFstg );

    fprintf( pFile, "product suppx.aut suppf.aut prod.aut\n" );
    fprintf( pFile, "support %s%s prod.aut prod.aut\n", pPriIns + 1, pPriOuts, pFileSstg );
    fprintf( pFile, "determinize prod.aut prod.aut\n" );

    fprintf( pFile, "read_blif %s\n", pNet->pSpec );
    fprintf( pFile, "extract_aut %s\n", pFileSstg );
    fprintf( pFile, "support %s%s %s supps.aut\n", pPriIns + 1, pPriOuts, pFileSstg );

    fprintf( pFile, "contain prod.aut supps.aut\n" );
    fprintf( pFile, "\n" );
    fclose( pFile );
    ////////////////////////////////////////////////////////////////////////////


    ////////////////////////////////////////////////////////////////////////////
    pFile = fopen( pFileScriptLC, "w" );
    fprintf( pFile, "echo \"Verifying the (monolithic) composition in the spec ... \"\n" );
    fprintf( pFile, "support %s%s%s%s %s suppx.aut\n", pPriIns + 1, pLatchesF? pLatchesF: "", 
        pLatchesX, pPriOuts, pFileZstg );

    fprintf( pFile, "read_blif %s\n", pFileF );
    fprintf( pFile, "latch_expose\n" );
    fprintf( pFile, "extract_aut %s\n", pFileFstg );
    fprintf( pFile, "support %s%s%s%s %s suppf.aut\n", pPriIns + 1, pLatchesF? pLatchesF: "", 
        pLatchesX, pPriOuts, pFileFstg );

    fprintf( pFile, "product suppx.aut suppf.aut prod.aut\n" );
    fprintf( pFile, "support %s%s prod.aut prod.aut\n", pPriIns + 1, pPriOuts, pFileSstg );
    fprintf( pFile, "determinize prod.aut prod.aut\n" );

    fprintf( pFile, "read_blif %s\n", pNet->pSpec );
    fprintf( pFile, "extract_aut %s\n", pFileSstg );
    fprintf( pFile, "support %s%s %s supps.aut\n", pPriIns + 1, pPriOuts, pFileSstg );

    fprintf( pFile, "contain prod.aut supps.aut\n" );
    fprintf( pFile, "\n" );
    fclose( pFile );
    ////////////////////////////////////////////////////////////////////////////


    if ( fVerbose )
    printf( "Script to solve F * X = S is written into file \"%s\".\n", pFileScriptS );
    if ( fVerbose )
    printf( "Running the script will produce the solution \"%s\".\n", pFileXstg );
    if ( fVerbose )
    printf( "and check that the solution contains the known implementation.\n" );


    FREE( pLatchesF );
    FREE( pLatchesX );
    FREE( pPriIns );
    FREE( pPriOuts );
    FREE( pNamesLatch );
    return 0;
}


#else

/**Function*************************************************************

  Synopsis    [Performs the latch splitting.]

  Description [Generates two files and the script to solve the 
  unknown component problem.]
               
  SideEffects []

  SeeAlso     [] 

***********************************************************************/
int Mvn_NetworkSplit2( Ntk_Network_t * pNet, char * pNamesLatchInit, char * pNamesOut, bool fVerbose )
{
    FILE * pFile;
    Ntk_Latch_t * pLatch;
    Ntk_Node_t * pNode;
    int FileType;
    char * pNamesLatch = util_strsav( pNamesLatchInit );
    char * pFileGeneric, * pFileExten;
    char pFileF[150], pFileA[150];
    char pFileScriptS[150], pFileScriptL[150], pFileScriptSC[150], pFileScriptLC[150];
    char pFileXstg[150], pFileFstg[150], pFileSstg[150], pFileZstg[150], pFileAstg[150];
    char * pLatchesF = NULL, * pLatchesX = NULL;
    char * pPriIns = NULL, * pPriOuts = NULL;
    char * pTemp;
    int * pLatches;
    int iLatch, nLatches, nLatchesX;

    // allocate room for the latch list
    nLatches  = Ntk_NetworkReadLatchNum( pNet );
    pLatches  = ALLOC( int, nLatches );
    nLatchesX = Mvn_NetworkReadLatchList( pNamesLatch, pLatches, nLatches );
    if ( nLatchesX == -1 )
    {
        free( pLatches );
        return 1;
    }
    if ( nLatchesX == 0 )
    {
        printf( "The number of latches of X is 0. Cannot proceed.\n" );
        return 1;
    }


    // get the file type
    FileType   = Ntk_NetworkIsBinary(pNet)?  IO_FILE_BLIF : IO_FILE_BLIF_MV;
    pFileExten = (FileType == IO_FILE_BLIF)? "blif": "mv";

    // get the file names
    pFileGeneric = Extra_FileNameGeneric( pNet->pSpec );
    sprintf( pFileF,        "%sf.%s", pFileGeneric, pFileExten );
    sprintf( pFileA,        "%sa.%s", pFileGeneric, pFileExten );
    sprintf( pFileAstg,     "%sa.%s", pFileGeneric, "aut" );
    sprintf( pFileFstg,     "%sf.%s", pFileGeneric, "aut" );
    sprintf( pFileSstg,     "%ss.%s", pFileGeneric, "aut" );
    sprintf( pFileXstg,     "%sxs.%s", pFileGeneric, "aut" );
    sprintf( pFileZstg,     "%sxl.%s", pFileGeneric, "aut" );
    sprintf( pFileScriptS,  "%sS.script", pFileGeneric );
    sprintf( pFileScriptL,  "%sL.script", pFileGeneric );
    sprintf( pFileScriptSC, "%sSC.script", pFileGeneric );
    sprintf( pFileScriptLC, "%sLC.script", pFileGeneric );
    FREE( pFileGeneric );


    // make the outputs of X look like PIs
    iLatch = 0; 
    Ntk_NetworkForEachLatch( pNet, pLatch )
        if ( pLatches[iLatch++] == 1 )
             Ntk_NodeSetSubtype( Ntk_LatchReadOutput(pLatch), MV_BELONGS_TO_NET );
    assert( iLatch == nLatches );

    // write the F network
    pTemp = pNet->pSpec;
    pNet->pSpec = pFileF;
    Io_WriteNetwork( pNet, pFileF, FileType, 1 );
    pNet->pSpec = pTemp;


    // change the outputs of the latches
    if ( fVerbose )
    printf( "The latches of the spec network \"%s\" are:\n", pNet->pSpec );
    iLatch = 0; 
    Ntk_NetworkForEachLatch( pNet, pLatch )
    {
        if ( pLatches[iLatch++] == 1 )
        {
            Ntk_NodeSetSubtype( Ntk_LatchReadOutput(pLatch), MV_BELONGS_TO_LATCH );
            pLatchesX = Extra_StringAppend( pLatchesX, "," );
            pLatchesX = Extra_StringAppend( pLatchesX, Ntk_LatchReadOutput(pLatch)->pName );
            if ( fVerbose )
            printf( " %d=%s(X)", iLatch - 1, Ntk_LatchReadOutput(pLatch)->pName );
        }
        else
        {
            Ntk_NodeSetSubtype( Ntk_LatchReadOutput(pLatch), MV_BELONGS_TO_NET );
            pLatchesF = Extra_StringAppend( pLatchesF, "," );
            pLatchesF = Extra_StringAppend( pLatchesF, Ntk_LatchReadOutput(pLatch)->pName );
            if ( fVerbose )
            printf( " %d=%s(F)", iLatch - 1, Ntk_LatchReadOutput(pLatch)->pName );
        }
        if ( fVerbose && iLatch % 5 == 0 )
            printf( "\n" );
    }
    assert( iLatch == nLatches );
    FREE( pLatches );

    if ( fVerbose && nLatches % 5 != 0 )
        printf( "\n" );

    if ( fVerbose )
    {
    printf( "PI = %d. ", Ntk_NetworkReadCiNum(pNet) - Ntk_NetworkReadLatchNum(pNet) );
    printf( "PO = %d. ", Ntk_NetworkReadCoNum(pNet) - Ntk_NetworkReadLatchNum(pNet) );
    printf( "Latch F = %d. ", Ntk_NetworkReadLatchNum(pNet) - nLatchesX );
    printf( "Latch X = %d. ", nLatchesX );
    printf( "U vars = %d. ", Ntk_NetworkReadCiNum(pNet) - nLatchesX );
    printf( "V vars = %d. ", nLatchesX );
    printf( "\n" );
    }


    // write the A network
    pTemp = pNet->pSpec;
    pNet->pSpec = pFileA;
    Io_WriteNetwork( pNet, pFileA, FileType, 1 );
    pNet->pSpec = pTemp;

    // change the outputs of the latches
    Ntk_NetworkForEachLatch( pNet, pLatch )
         Ntk_NodeSetSubtype( Ntk_LatchReadOutput(pLatch), MV_BELONGS_TO_LATCH );



    if ( fVerbose )
    printf( "Network F (fixed) is written into file \"%s\".\n", pFileF );
    if ( fVerbose )
    printf( "Network A (current implementation) is written into file \"%s\".\n", pFileA );


    // collect the PI names
    Ntk_NetworkForEachCi( pNet, pNode )
    {
        if ( pNode->Subtype == MV_BELONGS_TO_LATCH )
            continue;
        pPriIns = Extra_StringAppend( pPriIns, "," );
        pPriIns = Extra_StringAppend( pPriIns, pNode->pName );
    }

    // collect the PO names
    Ntk_NetworkForEachCo( pNet, pNode )
    {
        if ( pNode->Subtype == MV_BELONGS_TO_LATCH )
            continue;
        pPriOuts = Extra_StringAppend( pPriOuts, "," );
        pPriOuts = Extra_StringAppend( pPriOuts, pNode->pName );
    }

    // write the script files
    ////////////////////////////////////////////////////////////////////////////
    pFile = fopen( pFileScriptS, "w" );
    fprintf( pFile, "# Language solving script (partitioned) generated by MVSIS\n" );
    fprintf( pFile, "# for latch splitting of sequential network \"%s\" on %s\n", pNet->pSpec, Extra_TimeStamp() );
    fprintf( pFile, "# Command line was: \"_split %s %s\".\n", (fVerbose? "-v": ""), pNamesLatchInit );
    fprintf( pFile, "\n" );

    fprintf( pFile, "echo \"Solving the language equation ... \"\n" );
    fprintf( pFile, "solve %s %s %s%s %s %s\n", pFileF, pNet->pSpec, 
        pPriIns + 1, pLatchesF? pLatchesF: "", pLatchesX + 1, pFileXstg );
    fprintf( pFile, "psa %s\n", pFileXstg );
    fprintf( pFile, "\n" );
/*
    fprintf( pFile, "echo \"Verifying the containment of the known implementation ... \"\n" );
    fprintf( pFile, "read_blif %s\n", pFileA );
    fprintf( pFile, "latch_expose\n" );
    fprintf( pFile, "stg_extract %s\n", pFileAstg );
    fprintf( pFile, "support %s%s%s %s %s\n", pPriIns + 1, pLatchesF? pLatchesF: "", pLatchesX, pFileAstg, pFileAstg );
    fprintf( pFile, "check %s %s\n", pFileAstg, pFileXstg );
    fprintf( pFile, "\n" );
*/
    fclose( pFile );
    ////////////////////////////////////////////////////////////////////////////


    ////////////////////////////////////////////////////////////////////////////
    pFile = fopen( pFileScriptL, "w" );
    fprintf( pFile, "# Language solving script (monolithic) generated by MVSIS\n" );
    fprintf( pFile, "# for latch splitting of sequential network \"%s\" on %s\n", pNet->pSpec, Extra_TimeStamp() );
    fprintf( pFile, "# Command line was: \"_split %s %s\".\n", (fVerbose? "-v": ""), pNamesLatchInit );
    fprintf( pFile, "\n" );

    fprintf( pFile, "read_blif %s\n", pFileF );
    fprintf( pFile, "latch_expose\n" );
    fprintf( pFile, "stg_extract %s\n", pFileFstg );
    fprintf( pFile, "read_blif %s\n", pNet->pSpec );
    fprintf( pFile, "stg_extract %s\n", pFileSstg );
    fprintf( pFile, "\n" );

    fprintf( pFile, "echo \"Solving the equation using the standard script ... \"\n" );
    fprintf( pFile, "determinize -ci %s spec_dci.aut\n", pFileSstg );
    fprintf( pFile, "support %s%s%s%s spec_dci.aut suppsi.aut\n", pPriIns + 1, pLatchesF? pLatchesF: "", 
        pLatchesX, pPriOuts );
    fprintf( pFile, "support %s%s%s%s %s suppf.aut\n", pPriIns + 1, pLatchesF? pLatchesF: "", 
        pLatchesX, pPriOuts, pFileFstg );
    fprintf( pFile, "product suppf.aut suppsi.aut p.aut\n" );
    fprintf( pFile, "support %s%s%s p.aut p_supp.aut\n", pPriIns + 1, pLatchesF? pLatchesF: "", pLatchesX );
    fprintf( pFile, "determinize -ci p_supp.aut p_dci.aut\n" );
    fprintf( pFile, "progressive -i %d p_dci.aut %s\n", 
        Ntk_NetworkReadCiNum(pNet) - nLatchesX, pFileZstg );
    fprintf( pFile, "psa %s\n", pFileZstg );
    fprintf( pFile, "\n" );
/*
    fprintf( pFile, "echo \"Verifying the containment of the known implementation ... \"\n" );
    fprintf( pFile, "read_blif %s\n", pFileA );
    fprintf( pFile, "latch_expose\n" );
    fprintf( pFile, "stg_extract %s\n", pFileAstg );
    fprintf( pFile, "support %s%s%s %s %s\n", pPriIns + 1, pLatchesF? pLatchesF: "", pLatchesX, pFileAstg, pFileAstg );
    fprintf( pFile, "check %s %s\n", pFileAstg, pFileZstg );
    fprintf( pFile, "\n" );
*/
    fclose( pFile );
    ////////////////////////////////////////////////////////////////////////////


    ////////////////////////////////////////////////////////////////////////////
    pFile = fopen( pFileScriptSC, "w" );
    fprintf( pFile, "echo \"Verifying the (partitioned) composition in the spec ... \"\n" );
    fprintf( pFile, "support %s%s%s%s %s suppx.aut\n", pPriIns + 1, pLatchesF? pLatchesF: "", 
        pLatchesX, pPriOuts, pFileXstg );

    fprintf( pFile, "read_blif %s\n", pFileF );
    fprintf( pFile, "latch_expose\n" );
    fprintf( pFile, "stg_extract %s\n", pFileFstg );
    fprintf( pFile, "support %s%s%s%s %s suppf.aut\n", pPriIns + 1, pLatchesF? pLatchesF: "", 
        pLatchesX, pPriOuts, pFileFstg );

    fprintf( pFile, "product suppx.aut suppf.aut prod.aut\n" );
    fprintf( pFile, "support %s%s prod.aut prod.aut\n", pPriIns + 1, pPriOuts, pFileSstg );
    fprintf( pFile, "determinize prod.aut prod.aut\n" );

    fprintf( pFile, "read_blif %s\n", pNet->pSpec );
    fprintf( pFile, "stg_extract %s\n", pFileSstg );
    fprintf( pFile, "support %s%s %s supps.aut\n", pPriIns + 1, pPriOuts, pFileSstg );

    fprintf( pFile, "check prod.aut supps.aut\n" );
    fprintf( pFile, "\n" );
    fclose( pFile );
    ////////////////////////////////////////////////////////////////////////////


    ////////////////////////////////////////////////////////////////////////////
    pFile = fopen( pFileScriptLC, "w" );
    fprintf( pFile, "echo \"Verifying the (monolithic) composition in the spec ... \"\n" );
    fprintf( pFile, "support %s%s%s%s %s suppx.aut\n", pPriIns + 1, pLatchesF? pLatchesF: "", 
        pLatchesX, pPriOuts, pFileZstg );

    fprintf( pFile, "read_blif %s\n", pFileF );
    fprintf( pFile, "latch_expose\n" );
    fprintf( pFile, "stg_extract %s\n", pFileFstg );
    fprintf( pFile, "support %s%s%s%s %s suppf.aut\n", pPriIns + 1, pLatchesF? pLatchesF: "", 
        pLatchesX, pPriOuts, pFileFstg );

    fprintf( pFile, "product suppx.aut suppf.aut prod.aut\n" );
    fprintf( pFile, "support %s%s prod.aut prod.aut\n", pPriIns + 1, pPriOuts, pFileSstg );
    fprintf( pFile, "determinize prod.aut prod.aut\n" );

    fprintf( pFile, "read_blif %s\n", pNet->pSpec );
    fprintf( pFile, "stg_extract %s\n", pFileSstg );
    fprintf( pFile, "support %s%s %s supps.aut\n", pPriIns + 1, pPriOuts, pFileSstg );

    fprintf( pFile, "check prod.aut supps.aut\n" );
    fprintf( pFile, "\n" );
    fclose( pFile );
    ////////////////////////////////////////////////////////////////////////////


    if ( fVerbose )
    printf( "Script to solve F * X = S is written into file \"%s\".\n", pFileScriptS );
    if ( fVerbose )
    printf( "Running the script will produce the solution \"%s\".\n", pFileXstg );
    if ( fVerbose )
    printf( "and check that the solution contains the known implementation.\n" );


    FREE( pLatchesF );
    FREE( pLatchesX );
    FREE( pPriIns );
    FREE( pPriOuts );
    FREE( pNamesLatch );
    return 0;
}

#endif

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Mvn_NetworkReadLatchList( char * pNamesLatch, int * pLatchNums, int nLatches )
{
    char ** pTemps, * pTemp;
    int nParts, iNum1, iNum2;
    int nLatchesX, i, k;

    // set the latch list to -1
    for ( i = 0; i < nLatches; i++ )
        pLatchNums[i] = -1;

    // split the latch list into comma-sep parts
    pTemps = ALLOC( char *, nLatches );
    pTemp  = strtok( pNamesLatch, "," );
    for ( nParts = 0; pTemp; nParts++ )
    {
        pTemps[nParts] = pTemp;
        pTemp = strtok( NULL, "," );
    }

    // parse the parts
    for ( i = 0; i < nParts; i++ )
    {
        pTemp = strtok( pTemps[i], "-" );
        iNum1 = atoi( pTemp );
        pTemp = strtok( NULL, "-" );
        if ( pTemp == NULL )
        {
            if ( iNum1 >= nLatches )
            {
                printf( "Latch number %d is too large (can only be 0-%d).\n", iNum1, nLatches );
                free( pTemps );
                return -1;
            }
            if ( pLatchNums[iNum1] != -1 )
            {
                printf( "Number %d is listed multiple times in the latch list.\n", iNum1 );
                free( pTemps );
                return -1;
            }
            pLatchNums[iNum1] = 1;
        }
        else
        {
            iNum2 = atoi( pTemp );
            for ( k = iNum1; k <= iNum2; k++ )
            {
                if ( k >= nLatches )
                {
                    printf( "Latch number %d is too large (can only be 0-%d).\n", k, nLatches );
                    free( pTemps );
                    return -1;
                }
                if ( pLatchNums[k] != -1 )
                {
                    printf( "Number %d is listed multiple times in the latch list.\n", k );
                    free( pTemps );
                    return -1;
                }
                pLatchNums[k] = 1;
            }
        }
    }
    free( pTemps );

    // count the number of latches in F
    nLatchesX = 0;
    for ( i = 0; i < nLatches; i++ )
        if ( pLatchNums[i] == -1 )
            pLatchNums[i] = 0;
        else
            nLatchesX++;
    return nLatchesX;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


