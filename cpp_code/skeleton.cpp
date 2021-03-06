#include <vector>
#include <iostream>
#include <fstream>
#include <random>
#include <stdio.h>
#include <time.h>
#include <utility>
#include <cstring>
#include <ctime>
#include "find_intersections.cpp"
using namespace std;


struct SkeletonLine{
	int id;
	Point* start;
	Point* end;
	int type; //Options are 1 - 4
	bool pruned;
};

struct Polygon{
	vector<Point*> vertices;
	vector<Point*> auxVertices;
	vector<SkeletonLine*> skeleton;
	vector<int> convex;
	int num_convex;
	vector<int> convex_id;
	vector<pair<int,int>> steiner_edge;

	Polygon(){}
	Polygon(int n):
	vertices(n),
	convex(n)
	{};

	~Polygon(){
		for(int i = 0; i < vertices.size(); i++){
			delete vertices[i];
			vertices[i] = NULL;
		}
		for(int i = 0; i < auxVertices.size(); i++){
			delete auxVertices[i];
			auxVertices[i] = NULL;
		}
		for(int i = 0; i < skeleton.size(); i++){
			//delete skeleton[i]->start;
			//skeleton[i]->start = NULL;
			//delete skeleton[i]->end;
			//skeleton[i]->end = NULL;
			delete skeleton[i];
			skeleton[i] = NULL;
		}
		vertices.erase(vertices.begin(), vertices.end());
		auxVertices.erase(auxVertices.begin(), auxVertices.end());
		skeleton.erase(skeleton.begin(), skeleton.end());
		convex.erase(convex.begin(), convex.end());
		convex_id.erase(convex_id.begin(), convex_id.end());
		steiner_edge.erase(steiner_edge.begin(), steiner_edge.end());
	}
};

// Outputs polygon information to text files
void polygon_plotter(string filename, Polygon* polygon, bool heur){
	ofstream file;
	// Start with polygon file
	file.open("data/polygon/" + filename + "_polygon.txt");
	if(!file.is_open()){
		printf("Couldn't open polygon write to file\n");
	}
	file << "id x y convex\n";
	for(int i = 0; i < polygon->vertices.size(); i++){
		file << (i + 1) << " " << polygon->vertices[i]->x << " " << polygon->vertices[i]->y;
		if(polygon->convex[i] == 1){
		 	file << " TRUE\n";
		}else{
			file << " FALSE\n";
		}
	}
	file.close();
	// Now do the edge file
	// Not if heurisitc (don't want two files), should always call heuristic
	if(heur){
		file.open("data/polygon/" + filename + "_edges_heur.txt");
	}else{
		file.open("data/polygon/" + filename + "_edges.txt");
	}
	if(!file.is_open()){
		printf("Couldn't open edges write to file\n");
	}
	file << "id x y xend yend type intersect_prune\n";
	for(int i = 0; i < polygon->skeleton.size(); i++){
		file << i + 1 << " " << polygon->skeleton[i]->start->x << " " << polygon->skeleton[i]->start->y << " " << polygon->skeleton[i]->end->x << " " << polygon->skeleton[i]->end->y;
		file << " \"Type " << polygon->skeleton[i]->type << "\" ";
		if(polygon->skeleton[i]->pruned){
			file << "TRUE\n";
		}else{
			file << "FALSE\n";
		}
	}
	file.close();
}

// Outputs polygon information to text files in the form of the steiner tree problem
// Will have 4: One with only type 1,2 edge, one with types 1,2,3, one with all, one with all prunned
// Adding input to make my life easier
// Type 1: Only skeleton edges 1&2
// Type 2: Only skeleton edges 1,2&3
// Type 3: All skeleton edges
// Type 4: All skeleton edges, prunned
void stp_plotter(string filename, Polygon* polygon, int type){
	ofstream file;
	if(type < 0 && type > 4){
		printf("Wrong usage of function\n");
		return;
	}

	if(type == 0){file.open("data/STP_input/" + filename + "_STP_format_1-2.txt");}
	else if(type == 1){file.open("data/STP_input/" + filename + "_STP_format_1-2-3.txt");}
	else if(type == 2){file.open("data/STP_input/" + filename + "_STP_format_1-2-3_pruned.txt");}
	else if(type == 3){file.open("data/STP_input/" + filename + "_STP_format_all.txt");}
	else if(type == 4){file.open("data/STP_input/" + filename + "_STP_format_pruned.txt");}

	if(!file.is_open()){
		printf("Couldn't open STP write to file\n");
	}

	file << "33D32945 STP File, STP Format Version 1.0\n\n";
	file << "SECTION Comment\n";
	file << "Problem \"Classical Steiner tree problem in graph\"\n";

	if(type == 0){file << "Name " << filename << " only type 1&2 skeleton edges\n";}
	else if(type == 1){file << "Name " << filename << " only type 1,2&3 skeleton edges\n";}
	else if(type == 2){file << "Name " << filename << " only type 1,2&3 skeleton edges, pruned\n";}
	else if(type == 3){file << "Name " << filename << " all skeleton edges\n";}
	else if(type == 4){file << "Name " << filename << " all skeleton edges, pruned\n";}

	file << "Name " << filename << "\n";
	file << "Creator Nico Andres Thio\n";
	file << "END\n\n";
	file << "SECTION Graph\n";
	file << "Nodes " << polygon->num_convex + int(polygon->skeleton.size()) << "\n";
	file << "Edges " << int(polygon->steiner_edge.size()) << "\n";
	
	for(int i = 0; i < polygon->steiner_edge.size(); i++){
		if(type == 0 && 
			((polygon->steiner_edge[i].first > polygon->num_convex &&
				polygon->skeleton[polygon->steiner_edge[i].first - polygon->num_convex]->type >= 3) ||
			(polygon->steiner_edge[i].second > polygon->num_convex &&
				polygon->skeleton[polygon->steiner_edge[i].second - polygon->num_convex]->type >= 3))){
			continue;
		}
		else if(type == 1 && 
			((polygon->steiner_edge[i].first > polygon->num_convex &&
				polygon->skeleton[polygon->steiner_edge[i].first - polygon->num_convex]->type >= 4) ||
			(polygon->steiner_edge[i].second > polygon->num_convex &&
				polygon->skeleton[polygon->steiner_edge[i].second - polygon->num_convex]->type >= 4))){
			continue;
		}
		else if(type == 2 && 
			((polygon->steiner_edge[i].first > polygon->num_convex &&
				polygon->skeleton[polygon->steiner_edge[i].first - polygon->num_convex]->type >= 4) ||
			(polygon->steiner_edge[i].second > polygon->num_convex &&
				polygon->skeleton[polygon->steiner_edge[i].second - polygon->num_convex]->type >= 4))){
			continue;
		}
		else if(type == 2 &&
			((polygon->steiner_edge[i].first > polygon->num_convex && 
			polygon->skeleton[polygon->steiner_edge[i].first - polygon->num_convex]->pruned) ||
			(polygon->steiner_edge[i].second > polygon->num_convex && 
			polygon->skeleton[polygon->steiner_edge[i].second - polygon->num_convex]->pruned))){
			continue;
		}
		else if(type == 4 &&
			((polygon->steiner_edge[i].first > polygon->num_convex && 
			polygon->skeleton[polygon->steiner_edge[i].first - polygon->num_convex]->pruned) ||
			(polygon->steiner_edge[i].second > polygon->num_convex && 
			polygon->skeleton[polygon->steiner_edge[i].second - polygon->num_convex]->pruned))){
			continue;
		}

		// // Add a check to not add pruned
		// if((polygon->steiner_edge[i].first > polygon->num_convex && 
		// 	polygon->skeleton[polygon->steiner_edge[i].first - polygon->num_convex]->pruned) ||
		// 	(polygon->steiner_edge[i].second > polygon->num_convex && 
		// 	polygon->skeleton[polygon->steiner_edge[i].second - polygon->num_convex]->pruned)){
		// 	// printf("")
		// 	continue;
		// }
		file << "E " << polygon->steiner_edge[i].first + 1 << " " << polygon->steiner_edge[i].second + 1 << " 1\n";
	}
	file << "END\n";

	file << "\nSECTION Terminals\n";
	file << "Terminals " << polygon->num_convex << "\n";
	for(int i = 0; i < polygon->vertices.size(); i++){
		if(polygon->convex[i]){
			file << "T " << polygon->convex_id[i] + 1 << "\n";
		}
	}
	file << "END\n\n";
	file << "EOF\n";
	file.close();

}

// Just a printer function to make sure no colinear points
void check_colinear(Polygon* polygon){
	if(orientation(polygon->vertices.back(), polygon->vertices[0], polygon->vertices[1]) == 0){
			printf("Colinear 0\n");
	}
	if(orientation(polygon->vertices[polygon->vertices.size() - 2], polygon->vertices.back(), polygon->vertices[0]) == 0){
			printf("Colinear n\n");
	}
	for(int i =1; i < polygon->vertices.size() - 1; i++){
		if(orientation(polygon->vertices[i-1], polygon->vertices[i], polygon->vertices[i+1]) == 0){
			printf("Colinear %d\n",i);
		}
	}
}

// Uncrosses a pair of edges by changing the order of the points
void fix_crossing(Polygon* polygon, int index1, int index2){
	// Only need to reverse from index1 + 1 to index2 (index 2 is larger)
	int num = (index2 - index1) / 2;
	for(int i = 0; i < num; i++){
		// Switch the two
		Point* temp = polygon->vertices[index1 + i + 1];
		polygon->vertices[index1 + i + 1] = polygon->vertices[index2 - i];
		polygon->vertices[index2 - i] = temp;
	}
}

// Generate polygon obstacle
// Create n random points
// Uncross randomly chosen edges until no crossings
void generate_polygon(Polygon* polygon, int n, double X, double Y, bool rand, int seed){
	mt19937 mt_rand(seed);
	std::normal_distribution<double> randPosition(0, 1000000);
	// vector<Point> polygon(n);
	if(rand){
		for(int i = 0; i < n && rand; i++){
			// point =  new Point{i,
			// 		X * (double) mt_rand() / RAND_MAX,
			// 		Y * (double) mt_rand() / RAND_MAX};
					// static_cast <float> (rand() % 10),
					// static_cast <float> (rand() % 10)};
			polygon->vertices[i] = new Point{i,
					X * (double) mt_rand() / RAND_MAX,
					Y * (double) mt_rand() / RAND_MAX};
			// polygon->vertices[i] = new Point{i,
			// 		randPosition(mt_rand),
			// 		randPosition(mt_rand)};
		}
	}
	// printf("Here\n");
	// for(int i = 0; i < n; i++){
	// 	printf("(%.2f,%.2f)\n",polygon->vertices[i]->x,polygon->vertices[i]->y);
	// }
	n = polygon->vertices.size();
	// Uncross polygon i.e. find edges that intersect, and switch them
	bool crossings = true;
	while(crossings){
		crossings = false;
		// Pick random edges to start comparing from
		int firstStart = mt_rand() % n;
		int secondStart = mt_rand() % n;
		for(int i = 0; i < n && !crossings; i++){
			int i_index = (firstStart + i) % n;
			for(int j = 0; j < n && !crossings; j++){
				int j_index = (secondStart + j) % n;
				// If same or next to each other skip
				if(i_index == j_index || 
					min(i_index,j_index)+1 == max(i_index,j_index) ||
					(max(i_index,j_index) == n - 1 && min(i_index,j_index) == 0)){continue;}
				// if(doIntersect(&polygon[i_index], &polygon[(i_index + 1) % n], &polygon[j_index], &polygon[(j_index + 1) % n])){
				if(doIntersect(polygon->vertices[i_index], polygon->vertices[(i_index + 1) % n],
					polygon->vertices[j_index], polygon->vertices[(j_index + 1) % n])){
					// printf("Crossed %d (%.2f,%.2f) %d (%.2f,%.2f) and %d (%.2f,%.2f) %d (%.2f,%.2f)\n",
					// 	i_index, polygon->vertices[i_index]->x,polygon->vertices[i_index]->y,
					// 	(i_index + 1) % n, polygon->vertices[(i_index + 1) % n]->x,polygon->vertices[(i_index + 1) % n]->y,
					// 	j_index, polygon->vertices[j_index]->x,polygon->vertices[j_index]->y, 
					// 	(j_index + 1) % n, polygon->vertices[(j_index + 1) % n]->x,polygon->vertices[(j_index + 1) % n]->y);
					// printf("orientations: %d %d %d %d\n",orientation(polygon->vertices[i_index], polygon->vertices[(i_index + 1) % n], polygon->vertices[j_index]),
					// 										orientation(polygon->vertices[i_index], polygon->vertices[(i_index + 1) % n], polygon->vertices[(j_index + 1) % n]),
					// 										orientation(polygon->vertices[j_index], polygon->vertices[(j_index + 1) % n], polygon->vertices[i_index]),
					// 										orientation(polygon->vertices[j_index], polygon->vertices[(j_index + 1) % n], polygon->vertices[(i_index + 1) % n]));
					// cin.get();
					crossings = true;
					fix_crossing(polygon, min(i_index, j_index), max(i_index, j_index));
				}
			}
		}
	}
}

// Find which points are convex
// Point with min y coordinate must be convex
// Find it and go from there
void find_convex_points(Polygon* polygon){
	int minId = 0;
	// Point* minPoint = polygon->vertices[0];
	int n = polygon->vertices.size();
	// Find point with min y coordinate
	for(int i = 1; i < n; i++){
		if(polygon->vertices[minId]->y > polygon->vertices[i]->y){
			// printf("new id %d\n", i);
			// minPoint = polygon->vertices[i];
			// printf("min id %d\n", minPoint->id);
			minId = i;
		}
	}
	// printf("Min is %.4f %d\n",minPoint->y, minPoint->id);
	// This point must be convex
	int i = minId;
	polygon->convex[i] = true;
	int turn = orientation(polygon->vertices[(i - 1 + n) % n], polygon->vertices[i], polygon->vertices[(i + 1) % n]);
	bool add = true;
	int newTurn;
	// Now keep on cycling from this point
	i = (i + 1) % n;
	while(i != minId){
		newTurn = orientation(polygon->vertices[(i - 1 + n) % n], polygon->vertices[i], polygon->vertices[(i + 1) % n]);
		if((newTurn == turn && add) || 
			(newTurn != turn && !add)){
			polygon->convex[i] = 1;
			add = true;
		}else{
			polygon->convex[i] = 0;
			add = false;
		}
		turn = newTurn;
		i = (i + 1) % n;
	}
	polygon->num_convex = 0;
	for(int i = 0; i < n; i++){
		if(polygon->convex[i]){
			polygon->convex_id.push_back(polygon->num_convex);
		}else{
			polygon->convex_id.push_back(-1);
		}
		polygon->num_convex += polygon->convex[i];

	}
}

bool adjacent(int n, int i, int j){
	if(abs(i - j) == 1 || abs(i-j) == n - 1){return true;}
	return false;
}
// Returns angle between lines p->q and p->r
double line_angle(Point* p, Point* q, Point* r){
	// Get vectors
	Point line1 = {-1, q->x - p->x, q->y - p->y};
	Point line2 = {-1, r->x - p->x, r->y - p->y};
	double cosTheta = (line1.x * line2.x + line1.y * line2.y)/(sqrt(line1.x*line1.x + line1.y*line1.y)*sqrt(line2.x*line2.x + line2.y*line2.y));
	return 180*acos(cosTheta)/3.14;
	
}

// Cycle through edges of polygon, check if can see each other
// IMPORTANT: Assume i < j
bool can_see(Polygon* polygon, int i, int j){
	int n = polygon->vertices.size();
	// Check if line goes towards inside of polygon
	// Only need to check on of the endpoints,
	// But checking both might save some troubles
	// If convex, if different turn orientation or same 
	// but larger angle, outside polygon
	if(polygon->convex[i] == 1 && 
		(orientation(polygon->vertices[(i-1+n)%n], polygon->vertices[i], polygon->vertices[i+1]) != 
			orientation(polygon->vertices[(i-1+n)%n], polygon->vertices[i], polygon->vertices[j]) ||
			line_angle(polygon->vertices[i], polygon->vertices[(i-1+n)%n], polygon->vertices[i+1]) < 
			line_angle(polygon->vertices[i], polygon->vertices[(i-1+n)%n], polygon->vertices[j]))){

		return false;
	}
	if(polygon->convex[j] && 
		(orientation(polygon->vertices[j-1], polygon->vertices[j], polygon->vertices[(j+1) % n]) != 
			orientation(polygon->vertices[j-1], polygon->vertices[j], polygon->vertices[i]) ||
			line_angle(polygon->vertices[j], polygon->vertices[j-1], polygon->vertices[(j+1)%n]) < 
			line_angle(polygon->vertices[j], polygon->vertices[j-1], polygon->vertices[i]))){
		
		return false;
	}

	// If concave, same turn orientation and smaller angle, outside of polygon
	if(polygon->convex[i] == 0 &&
		orientation(polygon->vertices[(i-1+n)%n], polygon->vertices[i], polygon->vertices[i+1]) == 
			orientation(polygon->vertices[(i-1+n)%n], polygon->vertices[i], polygon->vertices[j]) &&
		line_angle(polygon->vertices[i], polygon->vertices[(i-1+n)%n], polygon->vertices[i+1]) > 
			line_angle(polygon->vertices[i], polygon->vertices[(i-1+n)%n], polygon->vertices[j])){

		return false;
	}
	if(!polygon->convex[j] &&
		orientation(polygon->vertices[j-1], polygon->vertices[j], polygon->vertices[(j+1) % n]) == 
			orientation(polygon->vertices[j-1], polygon->vertices[j], polygon->vertices[i]) &&
		line_angle(polygon->vertices[j], polygon->vertices[j-1], polygon->vertices[(j+1) % n]) > 
			line_angle(polygon->vertices[j], polygon->vertices[j-1], polygon->vertices[i])){
		return false;
	}

	// Check every edge for intersections
	for(int k = 0; k < n; k++){
		// if index is 1 smaller or same, skip (they share a point)
		if((i == 0 && k == n - 1) ||
			i - k == 1 ||
			i == k ||
			j - k == 1 ||
			j == k){continue;}
		// Now check if intersect, if so end
		if(doIntersect(polygon->vertices[i], polygon->vertices[j],
						polygon->vertices[k], polygon->vertices[(k+1) % n])){

			return false;
		}
	}

	return true;
}

void extend_skeleton_edge(Polygon* polygon, int edgeId, int i, int j){
	int n = polygon->vertices.size();
	Point* left = NULL;
	Point* right = NULL;
	Point* up = NULL;
	Point* down = NULL;
	Point* intersection;
	for(int k = 0; k < n; k++){
		// Ignore edges that share a node
		if((i == 0 && k == n - 1) ||
			i - k == 1 ||
			i == k ||
			j - k == 1 ||
			j == k){continue;}
		// Find intersection of lines (should exist)
		intersection = lineLineIntersection(polygon->vertices[i], polygon->vertices[j], polygon->vertices[k], polygon->vertices[(k+1) % n]);
		// If id of intersection is -1, means lines are parallel, skip
		if(intersection->id == -2){
			delete intersection;
			continue;
		}
		// Check if intersection is on line of boundary, must account for vertical lines
		if(isVertical(polygon->vertices[k],polygon->vertices[(k+1)%n])){
			if(intersection->y < min(polygon->vertices[k]->y,polygon->vertices[(k+1)%n]->y) ||
				intersection->y > max(polygon->vertices[k]->y,polygon->vertices[(k+1)%n]->y)){
				delete intersection;
				continue;
			}
		}else{
			if(intersection->x < min(polygon->vertices[k]->x,polygon->vertices[(k+1)%n]->x) ||
				intersection->x > max(polygon->vertices[k]->x,polygon->vertices[(k+1)%n]->x)){
				delete intersection;				
				continue;
			}
		}
		bool update = false;
		// Save if "better", must account for vertical lines
		if(isVertical(polygon->vertices[i],polygon->vertices[j])){
			if(intersection->y > max(polygon->vertices[i]->y,polygon->vertices[j]->y) &&
				(up == NULL || intersection->y < up->y)){
				delete up;
				up = intersection;
				update = true;
			}
			if(intersection->y < min(polygon->vertices[i]->y,polygon->vertices[j]->y) &&
				(down == NULL || intersection->y > down->y)){
				delete down;
				down = intersection;
				update = true;
			}
		}else{
			if(intersection->x > max(polygon->vertices[i]->x,polygon->vertices[j]->x) &&
				(right == NULL || intersection->x < right->x)){
				delete right;
				right = intersection;
				update = true;
			}
			if(intersection->x < min(polygon->vertices[i]->x,polygon->vertices[j]->x) &&
				(left == NULL || intersection->x > left->x)){
				delete left;
				left = intersection;
				update = true;
			}
		}
		if(!update){
			delete intersection;
		}	
	}
	// Here make sure save the correct ones
	// If type 3, expand both
	if(polygon->skeleton[edgeId]->type == 3){
		if(isVertical(polygon->vertices[i],polygon->vertices[j])){
			polygon->skeleton[edgeId]->start = up;
			polygon->skeleton[edgeId]->end = down;
			polygon->auxVertices.push_back(up);
			polygon->auxVertices.push_back(down);	
		}else{
			polygon->skeleton[edgeId]->start = left;
			polygon->skeleton[edgeId]->end = right;
			polygon->auxVertices.push_back(left);
			polygon->auxVertices.push_back(right);	
		}

	}
	// If type 2, expand only the concave one (should be j by default)
	if(polygon->skeleton[edgeId]->type == 2){
		if(isVertical(polygon->vertices[i],polygon->vertices[j])){
			if(polygon->skeleton[edgeId]->start->y < polygon->skeleton[edgeId]->end->y){
				polygon->skeleton[edgeId]->end = up;
				polygon->auxVertices.push_back(up);
				delete down;
			}else{
				delete up;
				polygon->skeleton[edgeId]->end = down;
				polygon->auxVertices.push_back(down);
			}
		}else{
			if(polygon->skeleton[edgeId]->start->x < polygon->skeleton[edgeId]->end->x){
				delete left;
				polygon->skeleton[edgeId]->end = right;
				polygon->auxVertices.push_back(right);
			}else{
				delete right;
				polygon->skeleton[edgeId]->end = left;
				polygon->auxVertices.push_back(left);
			}
		}
	}
}
// Need a slighlty different function, previous logic doesn't really apply
void extend_aux_edge(Polygon* polygon, int edgeId){
	int n = polygon->vertices.size();
	// Check which side we are extending on
	bool right = false, left = false, up = false, down = false;
	if(isVertical(polygon->skeleton[edgeId]->start, polygon->skeleton[edgeId]->end)){
		if(polygon->skeleton[edgeId]->start->y < polygon->skeleton[edgeId]->end->y){
			up = true;
		}else{
			down = true;
		}
	}else{
		if(polygon->skeleton[edgeId]->start->x < polygon->skeleton[edgeId]->end->x){
			right = true;
		}else{
			left = true;
		}
	}
	
	Point* extended = NULL;
	Point* intersection = NULL;
	for(int k = 0; k < n; k++){
		// Find intersection
		intersection = lineLineIntersection(polygon->skeleton[edgeId]->start, polygon->skeleton[edgeId]->end, polygon->vertices[k], polygon->vertices[(k+1) % n]);
		// If id of intersection is -1, means lines are parallel, skip
		if(intersection->id == -2){
			delete intersection;
			continue;
		}
		// Check if this is on the line, account for vertical lines
		if(isVertical(polygon->vertices[k],polygon->vertices[(k+1)%n])){
			if(intersection->y < min(polygon->vertices[k]->y,polygon->vertices[(k+1)%n]->y) ||
				intersection->y > max(polygon->vertices[k]->y,polygon->vertices[(k+1)%n]->y) ){
				delete intersection;				
				continue;
			}
		}else{
			if(intersection->x < min(polygon->vertices[k]->x,polygon->vertices[(k+1)%n]->x) ||
				intersection->x > max(polygon->vertices[k]->x,polygon->vertices[(k+1)%n]->x) ){
				delete intersection;				
				continue;
			}
		}
		
		// It is, check if better choice
		if(right && intersection->x > polygon->skeleton[edgeId]->end->x && 
			abs(intersection->x - polygon->skeleton[edgeId]->end->x) > 0.001 && 
			(extended == NULL || intersection->x < extended->x)){
			delete extended;
			extended = intersection;
		}
		else if(left && intersection->x < polygon->skeleton[edgeId]->end->x &&
			abs(intersection->x - polygon->skeleton[edgeId]->end->x) > 0.001 && 
			(extended == NULL || intersection->x > extended->x)){
			delete extended;			
			extended = intersection;
		}
		else if(up && intersection->y > polygon->skeleton[edgeId]->end->y &&
			abs(intersection->y - polygon->skeleton[edgeId]->end->y) > 0.001 && 
			(extended == NULL || intersection->y < extended->y)){
			delete extended;		
			extended = intersection;
		}
		else if(down && intersection->y < polygon->skeleton[edgeId]->end->y &&
			abs(intersection->y - polygon->skeleton[edgeId]->end->y) > 0.001 && 
			(extended == NULL || intersection->y > extended->y)){
			delete extended;	
			extended = intersection;
		}else{
			delete intersection;
		}
	}

	polygon->skeleton[edgeId]->end = extended;
	polygon->auxVertices.push_back(extended);

}

void add_aux_edge(Polygon* polygon, Point* start, Point* end, int side){
	// Cycle through vertices, see if this is an concave vertex than can be seen and has the right rotation
	int n = polygon->vertices.size();
	for(int i = 0; i < n; i++){
		// Only care if concave
		if(polygon->convex[i]){continue;}
		// Only are if counter-clockwise turn
		// Also should check boundary is to the right of line
		if(orientation(start, end, polygon->vertices[i]) != 2 ||
			orientation(end, polygon->vertices[i], polygon->vertices[(i + 1) % n]) != 2 ||
			orientation(end, polygon->vertices[i], polygon->vertices[(i - 1 + n) % n]) != 2 ||
			orientation(end, polygon->vertices[i], polygon->vertices[(i + 1) % n]) != side ||
			orientation(end, polygon->vertices[i], polygon->vertices[(i - 1 + n) % n]) != side){
			continue;
		}
		// if(orientation(end, polygon->vertices[i], polygon->vertices[(i + 1) % n]) != side ||
		// 	orientation(end, polygon->vertices[i], polygon->vertices[(i - 1 + n) % n]) != side){
		// 	continue;
		// }
		// if(orientation(start, end, polygon->vertices[i]) != 
		// 	orientation(end, polygon->vertices[i], polygon->vertices[(i + 1) % n]) || 
		// 	orientation(end, polygon->vertices[i], polygon->vertices[(i + 1) % n]) !=
		// 	orientation(end, polygon->vertices[i], polygon->vertices[(i - 1 + n) % n])){
		// 	continue;
		// }
		
		// If still here, need to see if can see the point.
		bool ok = true;
		for(int j = 0; j < n; j++){
			// Skip two lines (as intersect due to same points)
			if((i == 0 && j == n-1) ||
				i - j  == 1 ||
				i == j){
				continue;
			}
			if(doIntersect(end, polygon->vertices[i], polygon->vertices[j], polygon->vertices[(j + 1) % n])){
				// Check if this is line that end point is on (and should ignore)
				// Simply check if colinear
				if(orientation(end, polygon->vertices[j], polygon->vertices[(j + 1) % n]) == 0){
					continue;
				}
				// Otherwise this is a plain old intersection, discard this vertgex
				ok = false;
				break;
			}
		}
		if(ok){
			// Have a new edge, add it and extend it. Also need to call on the same procedure!
			polygon->skeleton.push_back(new SkeletonLine{int(polygon->skeleton.size()),end,polygon->vertices[i],4, false});
			extend_aux_edge(polygon, int(polygon->skeleton.size()) - 1);
			// And keep going!
			add_aux_edge(polygon, polygon->skeleton.back()->start, polygon->skeleton.back()->end, side);

		}
	}
}

void find_skeleton_edges(Polygon* polygon, bool heur){
	// First thing is to just check if can create all the edges that should be present
	int n = polygon->vertices.size();
	for(int i = 0; i < n; i++){
		for(int j = i + 1; j < n; j++){

			// Type 1: both convex
			if(polygon->convex[i] && polygon->convex[j]){
				// If can see, add to list. Cannot extend, so that is all
				if(adjacent(n,i,j) || can_see(polygon,i,j)){
					int newId = int(polygon->skeleton.size());
					polygon->skeleton.push_back(new SkeletonLine{newId,polygon->vertices[i],polygon->vertices[j],1, false});
					// Gotta add this as a STP edge for later
					polygon->steiner_edge.push_back(make_pair(polygon->convex_id[i], newId + polygon->num_convex));
					polygon->steiner_edge.push_back(make_pair(polygon->convex_id[j], newId + polygon->num_convex));
				}
			}
			// Type 2: one convex, one concave
			else if(polygon->convex[i] || polygon->convex[j]){
				// If can't see each other, stop
				if(!adjacent(n,i,j) && !can_see(polygon,i,j)){continue;}
				int concave, convex;
				if(polygon->convex[i]){
					convex = i;
					concave = j;
				}else{
					convex = j;
					concave = i;
				}
				// If not adjacent (i.e. can see), check if can extend
				// Note doing not adjacent because much cheaper check
				// If can't, stop
				if(!adjacent(n,i,j) && 
					orientation(polygon->vertices[convex], polygon->vertices[concave], polygon->vertices[(concave-1+n) % n]) !=
					orientation(polygon->vertices[convex], polygon->vertices[concave], polygon->vertices[(concave+1) % n])){
					continue;
				}
				// Time to add then extend
				int newId = int(polygon->skeleton.size());
				polygon->skeleton.push_back(new SkeletonLine{newId,polygon->vertices[convex],polygon->vertices[concave],2, false});
				extend_skeleton_edge(polygon, polygon->skeleton.size()-1, i, j);
				polygon->steiner_edge.push_back(make_pair(polygon->convex_id[convex], newId + polygon->num_convex));
				// Add possible aux edge
				// printf("Trying from (%.1f,%.1f) to (%.1f,%.1f)\n", polygon->vertices[convex]->x, polygon->vertices[convex]->y,
				// 													polygon->vertices[concave]->x, polygon->vertices[concave]->y);
				// Need orientation of turn
				int side;
				if(!adjacent(n,i,j) || convex != (concave-1+n) % n){
					side = orientation(polygon->vertices[convex], polygon->vertices[concave], polygon->vertices[(concave-1+n) % n]);
				}else{
					side = orientation(polygon->vertices[convex], polygon->vertices[concave], polygon->vertices[(concave+1) % n]);
				}
				if(!heur){add_aux_edge(polygon, polygon->skeleton.back()->start, polygon->skeleton.back()->end, side);}
			}
			// Type 3: Two concave on separate sides
			else{
				// Have two concave. Want them on separate sides.
				// If adjacent, this can't be the case
				if(adjacent(n,i,j)){continue;}
				// Check they can see each other
				if(!can_see(polygon,i,j)){continue;}
				// Check whether orientation is the same for all, if so we are good
				// So if different stop
				if(orientation(polygon->vertices[i], polygon->vertices[j], polygon->vertices[(j+1) % n]) != 
					orientation(polygon->vertices[i], polygon->vertices[j], polygon->vertices[j-1]) ||
					orientation(polygon->vertices[i], polygon->vertices[j], polygon->vertices[j-1]) != 
					orientation(polygon->vertices[j], polygon->vertices[i], polygon->vertices[i+1]) ||
					orientation(polygon->vertices[j], polygon->vertices[i], polygon->vertices[i+1]) != 
					orientation(polygon->vertices[j], polygon->vertices[i], polygon->vertices[(i-1+n)%n])){

					continue;
				}
				// Extend both sides, and add!
				polygon->skeleton.push_back(new SkeletonLine{int(polygon->skeleton.size()),polygon->vertices[i],polygon->vertices[j],3, false});
				extend_skeleton_edge(polygon, polygon->skeleton.size()-1, i, j);
				int currId = polygon->skeleton.size() - 1;
				if(!heur){
					add_aux_edge(polygon, polygon->skeleton[currId]->start, polygon->skeleton[currId]->end,
						orientation(polygon->vertices[i], polygon->vertices[j], polygon->vertices[j-1]));

					add_aux_edge(polygon, polygon->skeleton[currId]->end, polygon->skeleton[currId]->start,
						orientation(polygon->vertices[j], polygon->vertices[i], polygon->vertices[i+1]));
				}
			}

		}
	}
}	

int read_in_polygon(Polygon* polygon, string filename){
	FILE * fp;
	char buffer[1001];
	char buffer2[1001];
	char *cstr = new char[filename.length() + 1];
	strcpy(cstr, filename.c_str());
	fp = fopen(cstr, "r");
	if(!fp){
        printf("Could not open file.  Giving up.\n");
        return 1;
    }
    fgets(buffer, 1000, fp);
	fgets(buffer, 1000, fp);
	double x, y;
    int id = 0;
	while(feof(fp)==0){
		sscanf(buffer, "%lf %lf %s",&x,&y, buffer2);
		polygon->vertices.push_back(new Point{id,x,y});
		id++;
		polygon->convex.push_back(false);
		fgets(buffer, 1000, fp);
	}
	delete [] cstr;
	return 0;
}

void prune_skeleton_edges(Polygon* polygon){
	// Basically cycle through each pair of skeleton edges,
	// see if one of them intersects everything the other one 
	// intersects
	int n = polygon->skeleton.size();
	for(int i = 0; i < n; i++){
		for(int j = 0; j < n; j++){
			if(i == j){continue;}
			// Check if i can be pruned due to j
			// If j has been pruned, skip this
			if(polygon->skeleton[j]->pruned){continue;}
			// Some types allow us to skip
			if(polygon->skeleton[i]->type == 1){break;}
			// If this is type 2, this edge must also start or end on the convex vertex
			if(polygon->skeleton[i]->type == 2){
				if((abs(polygon->skeleton[i]->start->x - polygon->skeleton[j]->start->x) > 0.001 ||
					abs(polygon->skeleton[i]->start->y - polygon->skeleton[j]->start->y) > 0.001) &&
					(abs(polygon->skeleton[i]->start->x - polygon->skeleton[j]->end->x) > 0.001 ||
					abs(polygon->skeleton[i]->start->y - polygon->skeleton[j]->end->y) > 0.001)){
					continue;
				}
			}
			// Nothing left but to check agains every other edge
			// Keep looking until find an edge that intersect with i but not with j
			// If do not find such an edge, can prune i
			bool prune = true;
			for(int k = 0; k < n; k++){
				if(doIntersect(polygon->skeleton[i]->start, polygon->skeleton[i]->end,
					polygon->skeleton[k]->start, polygon->skeleton[k]->end) &&
					!doIntersect(polygon->skeleton[j]->start, polygon->skeleton[j]->end,
					polygon->skeleton[k]->start, polygon->skeleton[k]->end)){

					prune = false;
					break;
				}
			}
			if(prune){
				polygon->skeleton[i]->pruned = true;
				break;
			}

		}
	}
}

void stp_intersections(Polygon* polygon){
	int v = polygon->num_convex;
	int n = polygon->skeleton.size();
	for(int i = 0; i < n; i++){
		for(int j = i + 1; j < n; j++){
			if(doIntersect(polygon->skeleton[i]->start, polygon->skeleton[i]->end,
							polygon->skeleton[j]->start, polygon->skeleton[j]->end)){
				polygon->steiner_edge.push_back(make_pair(v + i,v + j));
			}
		}
	}
}

// Big function which does most of the work
// n is size of polygon
// seed is seed for random generator
// create_polygon whether to create or read polygon
// print_info whether to print times 
void create_skeleton(int n, int seed, bool heur, bool print_info, bool create_polygon, string filename){
	clock_t t, start;
	Polygon* polygon;
	ofstream file;
	t = clock();
	start = t;
	string name;

	if(create_polygon){
		file.open(filename, ios::out | ios::app);
		file << to_string(n) << " " << to_string(seed);
		if(heur){
			file << " TRUE";
		}else{
			file << " FALSE";
		}
		name = "random_n" + to_string(n) + "_s" + to_string(seed);
		polygon = new Polygon(n);
		generate_polygon(polygon, n, 1000000.0, 1000000.0, true, seed);
		t = clock() - t;
		if(print_info){printf("Created size %d instance:    %.5fs\n",n,((float)t)/CLOCKS_PER_SEC);}
		file << " " << to_string(((float)t)/CLOCKS_PER_SEC);
	}else{
		polygon = new Polygon();
		string input = "data/input_polygon/" + filename + "_polygon.txt";
		read_in_polygon(polygon, input);
		generate_polygon(polygon, int(polygon->vertices.size()), 1000000.0, 1000000.0, false, seed);
		name = filename;
		t = clock() - t;
		file << " FALSE " << to_string(((float)t)/CLOCKS_PER_SEC);
	}

	t = clock();
	find_convex_points(polygon);
	t = clock() - t;
	if(print_info){printf("Determined convex points:    %.5fs\n",((float)t)/CLOCKS_PER_SEC);}
	if(file.is_open()){file << " " << to_string(((float)t)/CLOCKS_PER_SEC);}

	t = clock();
	find_skeleton_edges(polygon, heur);
	t = clock() - t;
	if(print_info){printf("Found skeleton edges:        %.5fs\n",((float)t)/CLOCKS_PER_SEC);}
	if(file.is_open()){file << " " << to_string(((float)t)/CLOCKS_PER_SEC);}

	t = clock();
	prune_skeleton_edges(polygon);
	t = clock() - t;
	if(print_info){printf("Prunned polygon:             %.5fs\n",((float)t)/CLOCKS_PER_SEC);}
	if(file.is_open()){file << " " << to_string(((float)t)/CLOCKS_PER_SEC);}

	t = clock();
	stp_intersections(polygon);
	t = clock() - t;
	if(print_info){printf("Found STP intersections:     %.5fs\n",((float)t)/CLOCKS_PER_SEC);}
	if(file.is_open()){file << " " << to_string(((float)t)/CLOCKS_PER_SEC);}

	t = clock();
	polygon_plotter(name, polygon, heur);
	t = clock() - t;
	if(print_info){printf("Polygon info to text file:   %.5fs\n",((float)t)/CLOCKS_PER_SEC);}
	if(file.is_open()){file << " " << to_string(((float)t)/CLOCKS_PER_SEC);}
	t = clock();

	if(heur){
		stp_plotter(name, polygon, 0);
		stp_plotter(name, polygon, 1);
		stp_plotter(name, polygon, 2);
	}else{
		stp_plotter(name, polygon, 3);
		stp_plotter(name, polygon, 4);
	}

	t = clock() - t;
	if(print_info){printf("Polygon info to STP file:    %.5fs\n",((float)t)/CLOCKS_PER_SEC);}
	if(file.is_open()){file << " " << to_string(((float)t)/CLOCKS_PER_SEC);}

	t = clock() - start;
	if(print_info){printf("Total execution time:        %.5fs\n",((float)t)/CLOCKS_PER_SEC);}
	if(file.is_open()){file << " " << to_string(((float)t)/CLOCKS_PER_SEC) << "\n";}

	// Deal with memory
	delete polygon;

}

int main(int argc, char** argv){
	if(argc != 8 && argc != 3){
		printf("Wrong usage of exec file!\n");
		printf("To use, please call ./skeleton n_start n_end n_int seed_min seed_max heur print_info\n");
		printf("Where:\n");
		printf("       -n_start: int, min size of polygon instances\n");
		printf("       -n_end: int, max size of polygon instances\n");
		printf("       -n_int: int, interval of polygon instances size\n");
		printf("       -seed_min: int, min seed for random polygons analysed for each size\n");
		printf("       -seed_max: int, max seed for random polygons analysed for each size\n");
		printf("       -heur: true/false, whether to run heuristic approach (i.e. ignore type 4 edges).\n");
		printf("                          Will always run full approach regardless.\n");
		printf("       -print_info: true/false, whether to show details of execution\n");
		printf("Example: ./skeleton 10 80 10 50 true true\n");
		printf("\n\nAlternatively, you can simply specify a scenario name and whether to print info:\n");
		printf("./skeleton filename print_info\n");
		printf("Remember to put the file in data/R_polygon in this case\n\n");
		return 0;
	}

	// Assume inputs are correct, I am not that good. Read inputs
	string filename;
	clock_t t = clock();
	bool print_info;
	if(argc == 8){
		int start, end, interval, seed_min, seed_max;
		bool create_polygon, heur;
		start = stoi(argv[1]);
		end = stoi(argv[2]);
		interval = stoi(argv[3]);
		seed_min = stoi(argv[4]);
		seed_max = stoi(argv[5]);
		string s1(argv[7]);
		if(s1.compare("true") == 0){
			print_info = true;
		}else if(s1.compare("false") == 0){
			print_info = false;
		}else{
			printf("Please either use true or false for 5th input!\n");
			return 0;
		}

		string s2(argv[6]);
		if(s2.compare("true") == 0){
			heur = true;
		}else if(s2.compare("false") == 0){
			heur = false;
		}else{
			printf("Please either use true or false for 6th input!\n");
			return 0;
		}
		// So far so good, create file which will log the exec times
		string exec_times_filename = "data/exec_times/run_n" + to_string(start) + "<-" + to_string(interval) + "->" + to_string(end) + 
										"_s" + to_string(seed_min) + "<->" + to_string(seed_max) + ".txt";
		ofstream file;
		file.open(exec_times_filename);
		// file.open(exec_times_filename + ".txt");
		if(!file.is_open()){
			printf("Couldn't open exec_times_filename write to file\n");
			return 0;
		}
		file << "n seed heur create find_convex find_edges prune_edges STP_intersection polygon_file STP_file total\n";
		file.close();		
		for(int n = start; n <= end; n += interval){
			for(int seed = seed_min; seed <= seed_max; seed++){
				if(print_info){printf("\nn = %d seed = %d\n", n, seed);}
				create_skeleton(n,seed, false, print_info, true, exec_times_filename);
				if(heur){
					if(print_info){printf("\nn = %d seed = %d (heuristic)\n", n, seed);}
					create_skeleton(n,seed, true, print_info, true, exec_times_filename);
				}

			}
		}
		
	}

	if(argc == 3){
		string s1(argv[2]);
		if(s1.compare("true") == 0){
			print_info = true;
		}else if(s1.compare("false") == 0){
			print_info = false;
		}else{
			printf("Please either use true or false for 2nd input!\n");
			return 0;
		}
		string filename(argv[1]);
		create_skeleton(0,0, false, print_info,false,filename);
		create_skeleton(0,0, true, print_info,false,filename);
	}


	t = clock() - t;
	if(print_info){printf("======== Total time: %.5fs =======\n",((float)t)/CLOCKS_PER_SEC);}
	return 0;
}
