#include <vector>
#include <stdio.h>
#include <math.h>
#include <iostream>
#include <fstream>
#include <string>
// http://docs.oracle.com/cd/E17076_05/html/programmer_reference/arch_apis.html
#include <dbstl_vector.h>

#include "funPieceListLog.h"

// TODO Use BDB implementation of STL,
// https://blogs.oracle.com/berkeleydb/entry/the_c_standard_template_librar_1
// https://docs.oracle.com/cd/E17276_01/html/programmer_reference/stl.html
// https://docs.oracle.com/cd/E17076_04/html/api_reference/STL/frame_main.html

int main(int argc, char *argv[]){//data_count x 2
  if(argc != 3){
    std::cout << "usage: " << argv[0] << " data.bedGraph penalty\n";
    return 1;
  }
  std::ifstream bedGraph_file(argv[1]);
  if(!bedGraph_file.is_open()){
    std::cout << "Could not open data file\n";
    return 2;
  }
  std::string line;
  int chromStart, chromEnd, coverage, items, line_i=0;
  double min_log_mean=INFINITY, max_log_mean=-INFINITY, log_data;
  while(std::getline(bedGraph_file, line)){
    line_i++;
    items = sscanf(line.c_str(), "%d %d %d\n", &chromStart, &chromEnd, &coverage);
    log_data = log(coverage);
    if(log_data < min_log_mean){
      min_log_mean = log_data;
    }
    if(max_log_mean < log_data){
      max_log_mean = log_data;
    }
    if(items!=3){
      printf("error: unrecognized data on line %d\n", line_i);
      std::cout << line;
      return 3;
    }
  }
  printf("min_log_mean=%f max_log_mean=%f\n", min_log_mean, max_log_mean);
  return 0;
  double penalty;
  double *data_vec, *weight_vec, *mean_vec;
  int data_count;
  int *end_vec;
  //
  for(int data_i=1; data_i<data_count; data_i++){
    double log_data = log(data_vec[data_i]);
    if(log_data < min_log_mean){
      min_log_mean = log_data;
    }
    if(max_log_mean < log_data){
      max_log_mean = log_data;
    }
  }
  dbstl::db_vector<PiecewisePoissonLossLog> cost_model_mat(data_count * 2);
  PiecewisePoissonLossLog up_cost, down_cost, up_cost_prev, down_cost_prev;
  PiecewisePoissonLossLog min_prev_cost;
  int verbose=0;
  double cum_weight_i = 0.0, cum_weight_prev_i;
  for(int data_i=0; data_i<data_count; data_i++){
    cum_weight_i += weight_vec[data_i];
    if(data_i==0){
      // initialization Cdown_1(m)=gamma_1(m)/w_1
      down_cost.piece_list.emplace_back
	(1.0, -data_vec[0], 0.0,
	 min_log_mean, max_log_mean, -1, false);
    }else{
      // if data_i is up, it could have come from down_cost_prev.
      min_prev_cost.set_to_min_less_of(down_cost_prev, verbose);
      int status = min_prev_cost.check_min_of(down_cost_prev, down_cost_prev);
      if(status){
	printf("BAD MIN LESS CHECK data_i=%d status=%d\n", data_i, status);
	printf("=prev down cost\n");
	down_cost_prev.print();
	printf("=min less(prev down cost)\n");
	min_prev_cost.print();
	throw status;
      }
      // C^up_t(m) = (gamma_t + w_{1:t-1} * M^up_t(m))/w_{1:t}, where
      // M^up_t(m) = min{
      //   C^up_{t-1}(m),
      //   C^{<=}_down_{t-1}(m) + lambda/w_{1:t-1}
      // in other words, we need to divide the penalty by the previous cumsum,
      // and add that to the min-less-ified function, before applying the min-env.
      min_prev_cost.set_prev_seg_end(data_i-1);
      min_prev_cost.add(0.0, 0.0, penalty/cum_weight_prev_i);
      // if(data_i==2){
      // 	printf("computing cost data_i=%d\n", data_i);
      // 	verbose=1;
      // }else{
      // 	verbose=0;
      // }
      if(data_i==1){
	up_cost = min_prev_cost;
      }else{
	up_cost.set_to_min_env_of(min_prev_cost, up_cost_prev, verbose);
	status = up_cost.check_min_of(min_prev_cost, up_cost_prev);
	if(status){
	  printf("BAD MIN ENV CHECK data_i=%d status=%d\n", data_i, status);
	  printf("=prev down cost\n");
	  down_cost_prev.print();
	  printf("=min less(prev down cost) + %f\n", penalty);
	  min_prev_cost.print();
	  printf("=prev up cost\n");
	  up_cost_prev.print();
	  printf("=new up cost model\n");
	  up_cost.print();
	  throw status;
	}
      }
      up_cost.multiply(cum_weight_prev_i);
      up_cost.add
	(weight_vec[data_i],
	 -data_vec[data_i]*weight_vec[data_i],
	 0.0);
      up_cost.multiply(1/cum_weight_i);
      // compute down_cost.
      if(data_i==1){
	//for second data point, the cost is only a function of the
	//previous down cost (there is no first up cost).
	down_cost = down_cost_prev;
      }else{
	// if data_i is down, it could have come from up_cost_prev.
	min_prev_cost.set_to_min_more_of(up_cost_prev, verbose);
	status = min_prev_cost.check_min_of(up_cost_prev, up_cost_prev);
	if(status){
	  printf("BAD MIN MORE CHECK data_i=%d status=%d\n", data_i, status);
	  printf("=prev up cost\n");
	  up_cost_prev.print();
	  printf("=min more(prev up cost)\n");
	  min_prev_cost.print();
	  //throw status;
	}
	min_prev_cost.set_prev_seg_end(data_i-1);
	//NO PENALTY FOR DOWN CHANGE
	down_cost.set_to_min_env_of(min_prev_cost, down_cost_prev, verbose);
	status = down_cost.check_min_of(min_prev_cost, down_cost_prev);
	if(status){
	  printf("BAD MIN ENV CHECK data_i=%d status=%d\n", data_i, status);
	  printf("=prev up cost\n");
	  up_cost_prev.print();
	  printf("=min more(prev up cost)\n");
	  min_prev_cost.print();
	  printf("=prev down cost\n");
	  down_cost_prev.print();
	  printf("=new down cost model\n");
	  down_cost.print();
	  throw status;
	}
      }
      down_cost.multiply(cum_weight_prev_i);
      down_cost.add
	(weight_vec[data_i],
	 -data_vec[data_i]*weight_vec[data_i],
	 0.0);
      down_cost.multiply(1/cum_weight_i);
    }//if(data_i initialization else update
    cum_weight_prev_i = cum_weight_i;
    up_cost_prev = up_cost;
    down_cost_prev = down_cost;
    cost_model_mat[data_i] = up_cost;
    cost_model_mat[data_i + data_count] = down_cost;
  }
  // Decoding the cost_model_vec, and writing to the output matrices.
  double best_cost, best_log_mean, prev_log_mean;
  int prev_seg_end;
  int prev_seg_offset = 0;
  // last segment is down (offset N) so the second to last segment is
  // up (offset 0).
  down_cost = cost_model_mat[data_count*2-1];
  down_cost.Minimize
    (&best_cost, &best_log_mean,
     &prev_seg_end, &prev_log_mean);
  mean_vec[0] = exp(best_log_mean);
  end_vec[0] = prev_seg_end;
  int out_i=1;
  while(0 <= prev_seg_end){
    // up_cost is actually either an up or down cost.
    up_cost = cost_model_mat[prev_seg_offset + prev_seg_end];
    //printf("decoding out_i=%d prev_seg_end=%d prev_seg_offset=%d\n", out_i, prev_seg_end, prev_seg_offset);
    //up_cost.print();
    if(prev_log_mean != INFINITY){
      //equality constraint inactive
      best_log_mean = prev_log_mean;
    }
    up_cost.findMean
      (best_log_mean, &prev_seg_end, &prev_log_mean);
    mean_vec[out_i] = exp(best_log_mean);
    end_vec[out_i] = prev_seg_end;
    // change prev_seg_offset and out_i for next iteration.
    if(prev_seg_offset==0){
      //up_cost is actually up
      prev_seg_offset = data_count;
    }else{
      //up_cost is actually down
      prev_seg_offset = 0;
    }
    out_i++;
  }//for(data_i
}

