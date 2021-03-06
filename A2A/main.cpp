#include "distance.h"
#include<iomanip>
#include<sstream>
#include<unistd.h>
#include <sys/resource.h>
#include <stdlib.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <sys/times.h>
#include "geodesic_algorithm_subdivision.h"
#include "qtree.h"
#define qtimes 100
//-------------------------------------
// MAIN
//-------------------------------------
int query_type=0;
int clpsize=6;
int algo_type=0;
int k=3;
int i;
std::vector<double> xcor, ycor;
int x[upper_poi];
FILE *fp;
char prefix[255];
#ifndef WIN32
    double Time_preprocess=0;
    double  Time_dquery=0, Time_knnquery, Time_clpquery;
    double  Space_preprocess=0;
    double  Space_query=0;
    double errorbound_dis, errorbound_knn=0;
    struct rusage myTime_program_start, myTime_preprocess_end, myTime_query_begin, myTime_query_end;
#endif

void algo_1_2(geodesic::GeodesicAlgorithmExact &algorithm){
        fp=fopen("POINT.C","r");
        Node * rootNode = new Node;     //Initialised node
        setnode(rootNode, mesh.m_xmin, mesh.m_ymin, mesh.m_width, mesh.m_height);
        rootNode->mcode.clear();
        for(i=0;i<poi;i++)
        {
            if(fscanf(fp,"%d",&x[i]) != EOF)
            {
    //        printf("%f %f\n",rootNode->pointArray[i].x,rootNode->pointArray[i].y);
                rootNode->indexArray.push_back(x[i]);
            }
        }
       rootNode->size=i;
       fclose(fp);
       printf("file read finished\n");
      // Create the quadTree
#ifndef WIN32
       getrusage(RUSAGE_SELF,&myTime_program_start);
#endif

       BuildQuadTree(rootNode,mesh);
       PrintQuadTree(rootNode);//Added function to print for easier debugging

       Node * node1;
       Node * node2;

       node1=rootNode;
       node2=rootNode;
    
       std::cout<<"WSPD Construction Algorithm:"<<std::endl;
        switch(algo_type){
            case 1:
                generate_pair_eu(node1,node2);
                quadpairs.sort(compair_quadpair);
                break;
            case 2:
                generate_pair_geo(node1,node2,algorithm);
                quadpairs.sort(compair_quadpair);

        }
        std::cout<<"Preprocessing Finished."<<std::endl;
       /* std::ostringstream cmd;
        cmd.clear();
        cmd.str("");
        cmd << "/proc/" << getpid() << "/statm";
        fp=fopen(cmd.str().c_str(),"r");
        fscanf(fp,"%f", Space_preprocess);
        fclose(fp);*/
        Space_query=2*quadpairs.size()*(sizeof(Node)-upper_poi*sizeof(int)-upper_poi*sizeof(Point));
        if(algo_type==1)Space_preprocess=2*quadpairs.size()*sizeof(Node)+2*nodevector.size()*sizeof(Node);
        if(algo_type==2)Space_preprocess=2*quadpairs.size()*sizeof(Node)+sizeof(mesh)+sizeof(algorithm);
#ifndef WIN32
       getrusage(RUSAGE_SELF,&myTime_preprocess_end);
       if(algo_type==1)Time_preprocess=myTime_preprocess_end.ru_utime.tv_usec-myTime_program_start.ru_utime.tv_usec;
       if(algo_type==2)Time_preprocess=myTime_preprocess_end.ru_utime.tv_sec-myTime_program_start.ru_utime.tv_sec;
#endif
       PrintQuadPair();
       for(std::list<NodePair *>::iterator iten=quadpairs.begin();iten!=quadpairs.end();iten++){
            pairvector.push_back(*iten);
       } 
       for(int m=0;m<pairvector.size();m++){
           for(std::list<bool>::iterator bite=pairvector[m]->node1->mcode.begin();bite!=pairvector[m]->node1->mcode.end();bite++)
               std::cout<<*bite;
           std::cout<<", ";
           for(std::list<bool>::iterator bite=pairvector[m]->node2->mcode.begin();bite!=pairvector[m]->node2->mcode.end();bite++)
               std::cout<<*bite;
           std::cout<<std::endl;
       }
       for(int q=0;q<qtimes;q++){
       int qindex1=randn(nodevector.size());
       int qindex2=randn(nodevector.size());
       if(qindex1==qindex2)qindex2=(qindex1+1)%nodevector.size();
       for(std::list<bool>::iterator bite=nodevector[qindex1]->mcode.begin();bite!=nodevector[qindex1]->mcode.end();bite++)
           std::cout<<*bite;
       std::cout<<", ";
       for(std::list<bool>::iterator bite=nodevector[qindex2]->mcode.begin();bite!=nodevector[qindex2]->mcode.end();bite++)
           std::cout<<*bite;
       std::cout<<std::endl;
/*
 * Distance Query
 * */

#ifndef WIN32
       getrusage(RUSAGE_SELF,&myTime_query_begin);
#endif

       double distance_return=distance_query_eu(*nodevector[qindex1], *nodevector[qindex2]); 
       std::cout<<"distance_return:"<<distance_return<<std::endl;
#ifndef WIN32
       getrusage(RUSAGE_SELF,&myTime_query_end);
       Time_dquery+=myTime_query_end.ru_utime.tv_usec-myTime_query_begin.ru_utime.tv_usec;
#endif
       double realdistance;
       realdistance=distance_geo(nodevector[qindex1],nodevector[qindex2],algorithm);
       std::cout<<"realdistance:"<<realdistance<<std::endl;
       if(realdistance==0||realdistance<=distance_return)errorbound_dis+=0;
       else errorbound_dis+=std::abs((double)((distance_return/realdistance)-(double)1.0));
       std::cout<<errorbound_dis<<std::endl;

       /*
        * KNN Query
        * */

#ifndef WIN32
       getrusage(RUSAGE_SELF,&myTime_query_begin);
#endif
       distance_return=knn(*nodevector[qindex1],k);
       std::cout<<"distance_return:"<<distance_return<<std::endl;
#ifndef WIN32
       getrusage(RUSAGE_SELF,&myTime_query_end);
       Time_knnquery+=myTime_query_end.ru_utime.tv_usec-myTime_query_begin.ru_utime.tv_usec;
#endif
       realdistance=surface_knn(nodevector[qindex1]->indexArray[0],k);
       std::cout<<"realdistance:"<<realdistance<<std::endl;
       if(realdistance==0||realdistance>=distance_return)errorbound_knn+=0;
       else errorbound_knn+=std::abs((double)((distance_return/realdistance)-(double)1.0));
       std::cout<<errorbound_knn<<std::endl;

       /*
        * B-closest pair
        * */

#ifndef WIN32
       getrusage(RUSAGE_SELF,&myTime_query_begin);
#endif
       std::vector<Node*> v1;
       v1.clear();
       for(i=0;i<clpsize/2;i++){
           v1.push_back(nodevector[qindex1]);
           qindex1=(qindex1+1)%nodevector.size();
       }
       std::vector<Node*> v2;
       v2.clear();
       for(i=0;i<clpsize/2;i++){
           v2.push_back(nodevector[qindex2]);
           qindex2=(qindex2+1)%nodevector.size();
       }
    //   k_closest_pairs(v1,v2,k);
#ifndef WIN32
       getrusage(RUSAGE_SELF,&myTime_query_end);
       Time_clpquery+=myTime_query_end.ru_utime.tv_usec-myTime_query_begin.ru_utime.tv_usec;
#endif
       }
        
       errorbound_dis/=qtimes;
       errorbound_knn/=qtimes;
       
       fp=fopen("output.txt","a");
      fprintf(fp,"%f %f %f %f %f %f %f %f\n",Time_preprocess,Time_dquery, Time_knnquery, Time_clpquery, Space_preprocess, Space_query, errorbound_dis, errorbound_knn); 
       fclose(fp); 
       DeleteQuadTree(rootNode);
}

void algo_3(geodesic::GeodesicAlgorithmExact &algorithm){
        fp=fopen("POINT.C","r");
        GraphNode *rootGraph=new GraphNode();
        rootGraph->size=poi;
        rootGraph->parent=NULL;
        rootGraph->mcode.clear();
        for(i=0;i<poi;i++)
        {
            if(fscanf(fp,"%d",&x[i]) != EOF)
            {
                rootGraph->indexArray.push_back(x[i]);
                printf("x[%d]: %d\n",i,x[i]);
            }
        }
       fclose(fp);
       printf("file read finished\n");
#ifndef WIN32
       getrusage(RUSAGE_SELF,&myTime_program_start);
#endif
        BuildGraphTree(*rootGraph,mesh,algorithm);
        std::cout<<"Graph Tree Completed."<<std::endl;
        PrintGraphTree(*rootGraph,0);
        std::cout<<"Print Tree Completed."<<std::endl;
        generate_pair_geo(*rootGraph,*rootGraph,algorithm);
        graphpairs.sort(compair_graphpair);
        std::cout<<"Preprocessing Finished."<<std::endl;
       /* std::ostringstream cmd;
        cmd.clear();
        cmd.str("");
        cmd << "/proc/" << getpid() << "/statm";
        fp=fopen(cmd.str().c_str(),"r");
        fscanf(fp,"%f", Space_preprocess);
        fclose(fp);*/
        Space_query=2*graphpairs.size()*(sizeof(GraphNode)-upper_poi*sizeof(int));
        Space_preprocess=2*graphpairs.size()*sizeof(GraphNode)+sizeof(mesh)+sizeof(algorithm);
#ifndef WIN32
       getrusage(RUSAGE_SELF,&myTime_preprocess_end);
       Time_preprocess=myTime_preprocess_end.ru_utime.tv_sec-myTime_program_start.ru_utime.tv_sec;
#endif
       PrintGraphPair();
       for(std::list<GraphPair *>::iterator iten=graphpairs.begin();iten!=graphpairs.end();iten++){
            graphpairsvector.push_back(*iten);
       } 
       for(int m=0;m<graphpairsvector.size();m++){
           for(std::list<bool>::iterator bite=graphpairsvector[m]->node1->mcode.begin();bite!=graphpairsvector[m]->node1->mcode.end();bite++)
               std::cout<<*bite;
           std::cout<<", ";
           for(std::list<bool>::iterator bite=graphpairsvector[m]->node2->mcode.begin();bite!=graphpairsvector[m]->node2->mcode.end();bite++)
               std::cout<<*bite;
           std::cout<<std::endl;
       }
       for(int q=0;q<qtimes;q++){
       int qindex1=randn(graphnodevector.size());
       int qindex2=randn(graphnodevector.size());
       if(qindex1==qindex2)qindex2=(qindex1+1)%graphnodevector.size();
       for(std::list<bool>::iterator bite=graphnodevector[qindex1]->mcode.begin();bite!=graphnodevector[qindex1]->mcode.end();bite++)
           std::cout<<*bite;
       std::cout<<", ";
       for(std::list<bool>::iterator bite=graphnodevector[qindex2]->mcode.begin();bite!=graphnodevector[qindex2]->mcode.end();bite++)
           std::cout<<*bite;
       std::cout<<std::endl;

/*
 * Distance Query
 * */

#ifndef WIN32
       getrusage(RUSAGE_SELF,&myTime_query_begin);
#endif

       double distance_return=distance_query_graph(*graphnodevector[qindex1], *graphnodevector[qindex2]); 
       std::cout<<"distance_return:"<<distance_return<<std::endl;
#ifndef WIN32
       getrusage(RUSAGE_SELF,&myTime_query_end);
       Time_dquery+=myTime_query_end.ru_utime.tv_usec-myTime_query_begin.ru_utime.tv_usec;
#endif
       double realdistance;
       realdistance=distance_geo(*graphnodevector[qindex1],*graphnodevector[qindex2],algorithm);
       std::cout<<"realdistance:"<<realdistance<<std::endl;
       if(realdistance!=0&&realdistance<distance_return)errorbound_dis+=std::abs((double)((distance_return/realdistance)-(double)1.0));
       std::cout<<errorbound_dis<<std::endl;

       /*
        * KNN Query
        * */

#ifndef WIN32
       getrusage(RUSAGE_SELF,&myTime_query_begin);
#endif
       distance_return=knn(*graphnodevector[qindex1],k);
       std::cout<<"distance_return:"<<distance_return<<std::endl;
#ifndef WIN32
       getrusage(RUSAGE_SELF,&myTime_query_end);
       Time_knnquery+=myTime_query_end.ru_utime.tv_usec-myTime_query_begin.ru_utime.tv_usec;
#endif
       realdistance=surface_knn(graphnodevector[qindex1]->index,k);
       std::cout<<"realdistance:"<<realdistance<<std::endl;
       if(realdistance!=0&&realdistance<distance_return)errorbound_knn+=std::abs((double)((distance_return/realdistance)-(double)1.0));
       std::cout<<errorbound_knn<<std::endl;

       /*
        * B-closest pair
        * */

#ifndef WIN32
       getrusage(RUSAGE_SELF,&myTime_query_begin);
#endif
       std::vector<GraphNode*> v1;
       v1.clear();
       for(i=0;i<clpsize/2;i++){
           v1.push_back(graphnodevector[qindex1]);
           qindex1=(qindex1+1)%graphnodevector.size();
       }
       std::vector<GraphNode*> v2;
       v2.clear();
       for(i=0;i<clpsize/2;i++){
           v2.push_back(graphnodevector[qindex2]);
           qindex2=(qindex2+1)%graphnodevector.size();
       }
    //   k_closest_pairs(v1,v2,k);
#ifndef WIN32
       getrusage(RUSAGE_SELF,&myTime_query_end);
       Time_clpquery+=myTime_query_end.ru_utime.tv_usec-myTime_query_begin.ru_utime.tv_usec;
#endif
       }
       errorbound_dis/=qtimes;
       errorbound_knn/=qtimes;

       fp=fopen("output.txt","a");
      fprintf(fp,"%f %f %f %f %f %f %f %f\n",Time_preprocess,Time_dquery, Time_knnquery, Time_clpquery, Space_preprocess, Space_query, errorbound_dis, errorbound_knn); 
       fclose(fp); 
       DeleteGraphTree(*rootGraph);
}

void algo_4(geodesic::GeodesicAlgorithmExact &algorithm){
        std::vector<GeoNode*> AllPOI;
        double p2ptime=0;
        AllPOI.clear();
        std::vector<std::pair<int, GeoNode*> > POIs;
        POIs.clear(); 
        tml::qtree<float, int> tree(mesh.m_xmin, mesh.m_ymin, mesh.m_xmin+mesh.m_width, mesh.m_ymin+mesh.m_height);
      //  fp=fopen("POINT.C","r");
        for(i=0;i<mesh.vertices().size();i++)
        {
         //   if(fscanf(fp,"%d",&x[i]) != EOF)
         //   {
                GeoNode *n=new GeoNode(i,0);
                tree.add_node((&mesh.vertices()[i])->getx(), (&mesh.vertices()[i])->gety(), i);// (&mesh.vertices()[i])->id());
                AllPOI.push_back(n);
                std::pair<int , GeoNode*> m(i, n);
                POIs.push_back(m);
     //           printf("x[%d]: %d\n",i,x[i]);
         //   }
        }
      // fclose(fp);
       printf("file read finished\n");
#ifndef WIN32
       getrusage(RUSAGE_SELF,&myTime_program_start);
#endif
        double radius=0;
        double distance;
        stx::btree<int, GeoNode*> bplusgeotree(POIs.begin(), POIs.end());
	    geodesic::SurfacePoint source(&mesh.vertices()[0]);
        std::vector<geodesic::SurfacePoint> all_sources(1,source);
	    double const distance_limit = 0;
        geodesic::SurfacePoint target(&mesh.vertices()[1]);
	    std::vector<geodesic::SurfacePoint> stop_points(1, target);
        for(i=2;i<mesh.vertices().size();i++){
            stop_points.push_back(geodesic::SurfacePoint(&mesh.vertices()[i]));
        }
	    algorithm.propagate(all_sources, distance_limit, &stop_points);
        for(i=1;i<mesh.vertices().size();i++){
            geodesic::SurfacePoint p(&mesh.vertices()[i]);
            algorithm.best_source(p,distance);
            radius= std::max(distance, radius);
        }
        std::cout<<"radius stopdis: "<< radius <<" "<< algorithm.distance_stopped()<<std::endl;
        GeoNode rootGeo(0,radius);
        stop_points.clear();
        BuildGeoTree(rootGeo,mesh.vertices().size(),bplusgeotree,algorithm);
        std::cout<<"Tree Completed."<<std::endl;
     //   calculate_size(rootGeo);
      //  PrintGeoTree(rootGeo);
        
        generate_pair_geo(rootGeo, rootGeo, algorithm);
    //    generate_pair_geo_naive(rootGeo, rootGeo, algorithm);
        std::cout<<"Preprocessing Finished."<<std::endl;
       /* std::ostringstream cmd;
        cmd.clear();
        cmd.str("");
        cmd << "/proc/" << getpid() << "/statm";
        fp=fopen(cmd.str().c_str(),"r");
        fscanf(fp,"%f", Space_preprocess);
        fclose(fp);*/
        Space_query=geopairs.size()*(sizeof(float)+sizeof(int)+sizeof(bool)+2*sizeof(char))/(1024.0*1024.0);
//        Space_preprocess=2*geopairs.size()*sizeof(GeoNode)+sizeof(mesh)+sizeof(algorithm);
#ifndef WIN32
       getrusage(RUSAGE_SELF,&myTime_preprocess_end);
       Time_preprocess=myTime_preprocess_end.ru_utime.tv_sec-myTime_program_start.ru_utime.tv_sec;
#endif
     //  geopairs.sort(compair_geopair);
     //  PrintGeoPair();
       for(std::list<GeoPair *>::iterator iten=geopairs.begin();iten!=geopairs.end();iten++){
            geopairsvector.push_back(*iten);
       } 
/*       for(int m=0;m<geopairsvector.size();m++){
           for(std::list<int>::iterator bite=geopairsvector[m]->node1->mcode.begin();bite!=geopairsvector[m]->node1->mcode.end();bite++)
               std::cout<<*bite;
           std::cout<<", ";
           for(std::list<int>::iterator bite=geopairsvector[m]->node2->mcode.begin();bite!=geopairsvector[m]->node2->mcode.end();bite++)
               std::cout<<*bite;
           std::cout<<std::endl;
       }*/
            double posX1;// = mesh.m_xmin + mesh.m_width*((double) rand())/((double) RAND_MAX); 
            double posY1;// = mesh.m_ymin + mesh.m_height*((double) rand())/((double) RAND_MAX); 
            double posX2;// = mesh.m_xmin + mesh.m_width*((double) rand())/((double) RAND_MAX); 
            double posY2;// = mesh.m_ymin + mesh.m_height*((double) rand())/((double) RAND_MAX); 
       for(int q=0;q<2*qtimes;q++){
           if(q<qtimes){
            posX1 = mesh.m_xmin + mesh.m_width*((double) rand())/((double) RAND_MAX); 
            posY1 = mesh.m_ymin + mesh.m_height*((double) rand())/((double) RAND_MAX); 
            posX2 = mesh.m_xmin + mesh.m_width*((double) rand())/((double) RAND_MAX); 
            posY2 = mesh.m_ymin + mesh.m_height*((double) rand())/((double) RAND_MAX); 
           }
           else{
            posX1 = xcor[rand()*rand()%xcor.size()]; //mesh.m_xmin + mesh.m_width*((double) rand())/((double) RAND_MAX); 
            posY1 = ycor[rand()*rand()%ycor.size()]; //mesh.m_ymin + mesh.m_height*((double) rand())/((double) RAND_MAX); 
            posX2 = xcor[rand()*rand()%xcor.size()]; //mesh.m_xmin + mesh.m_width*((double) rand())/((double) RAND_MAX); 
            posY2 = ycor[rand()*rand()%ycor.size()]; //mesh.m_ymin + mesh.m_height*((double) rand())/((double) RAND_MAX); 
           }
        //tml::qtree<float, int> tree(mesh.m_xmin, mesh.m_ymin, mesh.m_xmin+mesh.m_width, mesh.m_ymin+mesh.m_height);
#ifndef WIN32
       getrusage(RUSAGE_SELF,&myTime_query_begin);
#endif
       int qindex1; //=randn(geonodevector.size());
       double NNradius = mesh.shortest_edge;
        const tml::qtree<float, int>::node_type *NNnode = NULL;
        while(NNnode==NULL){
            NNnode = tree.find_nearest(posX1, posY1, NNradius);// mesh.shortest_edge);
            NNradius*=2.0;
        }
        qindex1 = NNnode->data;
       NNradius = mesh.shortest_edge;
       NNnode = NULL;
        while(NNnode==NULL){
            NNnode = tree.find_nearest(posX2, posY2, NNradius);// mesh.shortest_edge);
            NNradius*=2.0;
        }
        int qindex2 = NNnode->data;
     //  int qindex2=randn(geonodevector.size());
       if(qindex1==qindex2)qindex2=(qindex1+1)%geonodevector.size();
/*       for(std::list<int>::iterator bite=geonodevector[qindex1]->mcode.begin();bite!=geonodevector[qindex1]->mcode.end();bite++)
           std::cout<<*bite;
       std::cout<<", ";
       for(std::list<int>::iterator bite=geonodevector[qindex2]->mcode.begin();bite!=geonodevector[qindex2]->mcode.end();bite++)
           std::cout<<*bite;
       std::cout<<std::endl;*/

/*
 * Distance Query
 * */


       double distance_return=distance_geo(geonodevector[qindex1], geonodevector[qindex2]); 
       std::cout<<"distance_return:"<<distance_return<<std::endl;
#ifndef WIN32
       getrusage(RUSAGE_SELF,&myTime_query_end);
       if(q<qtimes)Time_dquery+=myTime_query_end.ru_utime.tv_usec-myTime_query_begin.ru_utime.tv_usec;
       else p2ptime+=myTime_query_end.ru_utime.tv_usec-myTime_query_begin.ru_utime.tv_usec;
#endif
       double realdistance;
       realdistance=distance_geo(*geonodevector[qindex1],*geonodevector[qindex2],algorithm);
       std::cout<<"realdistance:"<<realdistance<<std::endl;
       if(realdistance!=0&&distance_return>realdistance)errorbound_dis+=std::abs((double)((distance_return/realdistance)-(double)1.0));
       std::cout<<errorbound_dis<<std::endl;

/*
 * KNN Query
 * */

/*#ifndef WIN32
       getrusage(RUSAGE_SELF,&myTime_query_begin);
#endif
       distance_return=knn(*geonodevector[qindex1],k);
       std::cout<<"distance_return:"<<distance_return<<std::endl;
#ifndef WIN32
       getrusage(RUSAGE_SELF,&myTime_query_end);
       Time_knnquery+=myTime_query_end.ru_utime.tv_usec-myTime_query_begin.ru_utime.tv_usec;
#endif
       realdistance=surface_knn(geonodevector[qindex1]->index,k);
       std::cout<<"realdistance:"<<realdistance<<std::endl;
       if(realdistance!=0&&realdistance<distance_return)errorbound_knn+=std::abs((double)((distance_return/realdistance)-(double)1.0));
       std::cout<<errorbound_knn<<std::endl;
*/
       /*
        * B-closest pair
        * */
/*
#ifndef WIN32
       getrusage(RUSAGE_SELF,&myTime_query_begin);
#endif
       std::vector<GeoNode*> v1;
       v1.clear();
       for(i=0;i<clpsize/2;i++){
           v1.push_back(geonodevector[qindex1]);
           qindex1=(qindex1+1)%geonodevector.size();
       }
       std::vector<GeoNode*> v2;
       v2.clear();
       for(i=0;i<clpsize/2;i++){
           v2.push_back(geonodevector[qindex2]);
           qindex2=(qindex2+1)%geonodevector.size();
       }*/
   //    k_closest_pairs(v1,v2,k);
//#ifndef WIN32
//       getrusage(RUSAGE_SELF,&myTime_query_end);
  //     Time_clpquery+=myTime_query_end.ru_utime.tv_usec-myTime_query_begin.ru_utime.tv_usec;
//#endif
       }
       errorbound_dis/=qtimes;
       Time_dquery/=qtimes;
       p2ptime/=qtimes;
       
    //   errorbound_knn/=qtimes;
//       fp=fopen("output.txt","a");
//      fprintf(fp,"%f %f %f %f %f %f %f %f\n",Time_preprocess,Time_dquery, Space_query, errorbound_dis); 
//       fclose(fp); 
       std::ofstream disquerytime("SE.txt", std::ios::out | std::ios::app );
        disquerytime << 2.0/s << " " << xcor.size() << " "  << Time_preprocess << " " << std::setprecision(10)  << p2ptime << " " << Space_query << " " << errorbound_dis << " " << std::setprecision(10) << Time_dquery << std::endl; 
        DeleteGeoTree(rootGeo);
}
/*-------------------------------------------------------------------------
 * ------------------------MAIN--------------------------------------------
 *  -------------------------------------------------------------------------
 *
 * */


int main(int argc, char **argv) 
{
	if(argc < 2)
	{
		std::cout << "usage: mesh_file_name " << std::endl; //try: "hedgehog_mesh.txt 3 14" or "flat_triangular_mesh.txt 1"
		return 0;
	}

	bool success = geodesic::read_mesh_from_file(argv[1],points,faces);
	if(!success)
	{
		std::cout << "something is wrong with the input file" << std::endl;
		return 0;
	}
//    sscanf(argv[2], "%f", &s );
    s = atof(argv[2]);
	mesh.initialize_mesh_data(points, faces);		//create internal mesh data structure including edges
    strcpy(prefix, argv[1]);
    std::ifstream points(argv[3], std::ios::in);

    geodesic::GeodesicAlgorithmExact algorithm(&mesh);	//create exact algorithm for the mesh
    std::cout << s << std::endl;
    while(!points.eof()){
        double xx, yy;
        points >> xx >> yy;
//        std::cout << xx << " " << yy << std::endl;
        if(points.eof()) break;
        xcor.push_back(xx);
        ycor.push_back(yy);
    }
	//geodesic::GeodesicAlgorithmSubdivision subdivision_algorithm(&mesh,2);	//with subdivision_level=0 this algorithm becomes Dijkstra, with subdivision_level->infinity it becomes exact
    // WRITE THE RANDOM POINT FILE 
/*    fp = fopen("POINT.C","w");
    if ( fp == NULL )
    {
        puts ( "Cannot open file" );
        exit(1);
    }
    for(i=0;i<poi;i++)
    {
        bool key=true;
        while(key){
            key=false;
            x[i]=randn(mesh.vertices().size());
            for(int j=0;j<i;j++){
                if(x[i]==x[j]){
                    key=true;
                    break;
                }
            }
        }
        fprintf(fp,"%d\n",x[i]);
    }
    fclose(fp);*/
// READ THE RANDOM POINT FILE AND ASSIGN TO ROOT Node
    std::cout<<"Tree Building:"<<std::endl;
    std::cout<<"1: 2D QuadTree"<<std::endl;
    std::cout<<"2: Geodesic Tree"<<std::endl;
    std::cout<<"3: Graph Tree"<<std::endl;
//    fp=fopen("output.txt","w");
//    fclose(fp);
    for(algo_type=4;algo_type<5;algo_type++){
    pairs=0;
    quadpairs.clear();
    pairvector.clear();
    nodevector.clear();
    geopairs.clear();
    geopairsvector.clear();
    geonodevector.clear();
    graphpairs.clear();
    graphpairsvector.clear();
    graphnodevector.clear();
    std::cout<<"----------Algorithm "<<algo_type<<"------------"<<std::endl;

    if(algo_type==1||algo_type==2)algo_1_2(algorithm);
    if(algo_type==3){
        algo_3(algorithm);
    }
    if(algo_type==4){
        algo_4(algorithm);
    }
    }
}
