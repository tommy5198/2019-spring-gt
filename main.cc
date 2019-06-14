/* 
    Your main program goes here
*/
#include <iostream>
#include "network_manager.h"
#include "edge.h"
#include "vertex.h"
#include "gplot.h"
#include "path.h"
#include <map>
#include <vector>
#include <string>
#include <set>
#include <queue>
#include <deque>
#include <utility>

using namespace std;


#define MAX 1e9

NetworkManager *nm;
deque<Edge *> resultEdge;
map<Vertex *, vector<Edge *>> vlist;
    map<string, map<string, vector<pair<string, int>>>> spMap;

void findEulerCircuit(Vertex *x)
{
    for(int i = 0; i < vlist[x].size(); i++) {
        Edge *e = vlist[x][i];
        if(e && e->cap) {
            e->cap--;
            findEulerCircuit(e->tail);
            resultEdge.push_front(e);
            cout << e->flowval << " " << e->head->name << " -> " << e->tail->name << endl;
            cout << spMap[e->head->name][e->tail->name].size() << endl;
        }
    }
}

int main(int argc, char** argv)
{

    /* start your program */

    if(argc != 3) {
        cout << "wrong arguments! \nusage: main.out <input_filename> <output_filename>" << endl;
        return 1;
    }

    nm = new NetworkManager();
    nm->interpret(argv[1]);
    Path *sp = new Path();
    sp->append(nm->elist);
    Vertex *vlst = nm->get_all_nodes();
    bool isSCC = true;
    for(Vertex *a = vlst; isSCC && a != NULL; a = a->next)
        for(Vertex *b = a->next; isSCC && b != NULL; b = b->next) 
            if(sp->find_paths(a->name, b->name).empty() || sp->find_paths(b->name, a->name).empty())
                isSCC = false;
        
    if(!isSCC) {
        cout << "Impossible!" << endl;
        return 0;
    }
    map<Vertex *, int> diff; 

    for(Edge *tmp = nm->elist; tmp; tmp = tmp->next) {
        diff[tmp->head]--;
        diff[tmp->tail]++;
        tmp->flowval = tmp->cap;
        tmp->cap = 1;
        vlist[tmp->head].push_back(tmp);
    }
    vector<Vertex *> plusNode;
    vector<Vertex *> minusNode; 
    NetworkManager *mcmf = new NetworkManager();
    map<Vertex *, vector<Vertex *>> adj;
    map<Vertex *, int> d;
    mcmf->add_switch("src");
    mcmf->add_switch("dst");
    Vertex *src = mcmf->get_node("src");
    Vertex *dst = mcmf->get_node("dst");
    for (auto iter = diff.begin(); iter != diff.end(); ++iter) {
        Vertex *cur = iter->first;
        int curDiff = iter->second;

        if(!curDiff)
            continue;

        Edge *tmp = new Edge();
        mcmf->add_vertex(cur);
        if(curDiff > 0) {
            plusNode.push_back(cur);
            tmp->link(src, cur);
            adj[src].push_back(cur);
            for (auto nt = minusNode.begin(); nt != minusNode.end(); ++nt) {
                Vertex *sec = *nt;
                sp->find_paths(cur->name, sec->name);
                int minL = MAX;
                for(int i = 0; i < sp->paths.size(); i++) {
                    int tmpL = 0;
                    vector<pair<string, int>> v;
                    v.push_back(make_pair(cur->name, 0));
                    for(int j = 0; j < sp->paths.at(i).size(); j++) {
                        tmpL += sp->paths.at(i).at(j)->flowval;
                        v.push_back(make_pair(sp->paths[i][j]->tail->name, sp->paths[i][j]->flowval));
                    }
                    if(minL > tmpL) {
                        spMap[cur->name][sec->name].assign(v.begin(), v.end());
                        minL = tmpL;
                    }
                }
                Edge *intra = new Edge();
                intra->link(cur, sec);
                adj[cur].push_back(sec);
                intra->set_cap(MAX);
                intra->set_flowval(minL);
                mcmf->add_edge(intra);
                intra = new Edge();
                intra->link(sec, cur);
                adj[sec].push_back(cur);
                intra->set_cap(0);
                intra->set_flowval(-minL);
                mcmf->add_edge(intra);
            } 
            tmp->set_cap(curDiff);
            tmp->set_flowval(0);
            mcmf->add_edge(tmp);
            tmp = new Edge();
            tmp->link(cur, src);
            adj[cur].push_back(src);
            tmp->set_cap(0);
            tmp->set_flowval(0);
            mcmf->add_edge(tmp);
        } else if(curDiff < 0) {
            minusNode.push_back(cur); 
            tmp->link(cur, dst);
            adj[cur].push_back(dst);
            for (auto nt = plusNode.begin(); nt != plusNode.end(); ++nt) {
                Vertex *sec = *nt;
                sp->find_paths(sec->name, cur->name);
                int minL = MAX;
                for(int i = 0; i < sp->paths.size(); i++) {
                    int tmpL = 0;
                    vector<pair<string, int>> v;
                    v.push_back(make_pair(cur->name, 0));
                    for(int j = 0; j < sp->paths.at(i).size(); j++) {
                        tmpL += sp->paths.at(i).at(j)->flowval;
                        v.push_back(make_pair(sp->paths[i][j]->tail->name, sp->paths[i][j]->flowval));
                    }
                    if(minL > tmpL) {
                        spMap[sec->name][cur->name].assign(v.begin(), v.end());
                        minL = tmpL;
                    }
                }
                Edge *intra = new Edge();
                intra->link(sec, cur);
                adj[sec].push_back(cur);
                intra->set_cap(MAX);
                intra->set_flowval(minL);
                mcmf->add_edge(intra);
                intra = new Edge();
                intra->link(cur, sec);
                adj[cur].push_back(sec);
                intra->set_cap(0);
                intra->set_flowval(-minL);
                mcmf->add_edge(intra);
            } 
            tmp->set_cap(-curDiff);
            tmp->set_flowval(0);
            mcmf->add_edge(tmp);
            tmp = new Edge();
            tmp->link(dst, cur);
            adj[dst].push_back(cur);
            tmp->set_cap(0);
            tmp->set_flowval(0);
            mcmf->add_edge(tmp);
        }

    }

    // matching edge
    
    int flow = 0;
    int cost = 0;

    map<Vertex *, Vertex *> extraPath;
    set<Vertex *> inQ;

    while(1) {
        
        d[src] = 0;
        d[dst] = MAX;

        for(int i = 0; i < plusNode.size(); i++)
            d[plusNode[i]] = MAX;
        for(int i = 0; i < minusNode.size(); i++)
            d[minusNode[i]] = MAX;
        queue<Vertex *> Q;
        Q.push(src);

        while(!Q.empty()) {
            Vertex *a = Q.front();
            Q.pop();
            inQ.erase(a);
            for (auto iter = adj[a].begin(); iter != adj[a].end(); ++iter) {
                Vertex *b = *iter;
                Edge *e = mcmf->get_edge(a, b);
                if(e->cap > 0 && d[a] + e->flowval < d[b]) {
                    d[b] = d[a] + e->flowval;
                    extraPath[b] = a;
                    if(inQ.find(b) == inQ.end()) {
                        Q.push(b);
                        inQ.insert(b);
                    }
                }
            } 
        }
        if(d[dst] == MAX) break;

        int df = MAX;
        for(Vertex *a = dst; a != src; a = extraPath[a]) {
            Edge *e = mcmf->get_edge(extraPath[a], a);
            df = df < e->cap ? df : e->cap;
        }
        for(Vertex *a = dst; a != src; a = extraPath[a]) {
            Edge *e = mcmf->get_edge(extraPath[a], a);
            e->cap -= df;
            e = mcmf->get_edge(a, extraPath[a]);
            e->cap += df;
        }
        flow += df;
        cost += df * d[dst];
    }
    
    for (auto iter = plusNode.begin(); iter != plusNode.end(); ++iter) {
        for (auto til = minusNode.begin(); til != minusNode.end(); ++til) {
            Edge *e = mcmf->get_edge(*iter, *til);
            if(e->cap != MAX) {
                e->cap = MAX - e->cap;
                e->flowval = -e->flowval;
                nm->add_edge(e);
                vlist[e->head].push_back(e);
            }           
        }  
    } 

    resultEdge.clear();
    int totCost = 0;
    int cnt = 1;
    Edge *resultEdgeHead = NULL;
    Edge *resultEdgeList;

     

    findEulerCircuit(nm->elist->head);
    
    cout << "routing result: " << nm->elist->head->name;
    for (auto iter = resultEdge.begin(); iter != resultEdge.end(); ++iter) { 
        Edge *e = *iter;
        if(e->flowval > 0) {
            cout << " -> " << e->tail->name;
            totCost += e->flowval;
            if(resultEdgeHead == NULL) {
                resultEdgeHead = new Edge(e);
                resultEdgeList = resultEdgeHead;
            } else {
                resultEdgeList->next = new Edge(e);
                resultEdgeList = resultEdgeList->next;
            }
            resultEdgeList->cap = cnt++;
        } else {
            string pre = spMap[e->head->name][e->tail->name][0].first;
            for(int i = 1; i < spMap[e->head->name][e->tail->name].size(); i++) {
                string zz = spMap[e->head->name][e->tail->name][i].first;
                int zLen = spMap[e->head->name][e->tail->name][i].second;
                cout << " x-> " << zz;
                
                if(resultEdgeHead == NULL) {
                    resultEdgeHead = new Edge();
                    resultEdgeList = resultEdgeHead;
                } else {
                    resultEdgeList->next = new Edge();
                    resultEdgeList = resultEdgeList->next;
                }
                resultEdgeList->head = new Vertex();
                resultEdgeList->tail = new Vertex();
                resultEdgeList->head->name = pre;
                resultEdgeList->tail->name = zz;
                pre = zz;
                resultEdgeList->cap = cnt++;
                resultEdgeList->flowval = zLen;
            }
            totCost -= e->flowval;  
        }
    }
    cout << endl << "total cost: " << totCost << endl;
    
    for(Edge *e = resultEdgeHead; e; e = e->next)
        cout << e->head->name << " -> " << e->tail->name <<endl;
    
    Gplot *gp = new Gplot();
    gp->gp_add(resultEdgeHead);
    gp->gp_dump(true);
    gp->gp_export(argv[2]);



    return 0;
}

