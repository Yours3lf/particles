#pragma once

#include <functional>

//TODO
//stretching by velocity
//inherit velocity
//soft particles
//instancing
//collision

enum animable_type
{
  CONSTANT = 0, FUNCTION
};

template< class t >
struct animable_property
{
  animable_type type;

  t value;
  std::function<t( float, const vec3&, const vec3& )> func;

  t get( float a, const vec3& b, const vec3& c )
  {
    if( type == CONSTANT )
    {
      return value;
    }
    else
    {
      return func( a, b, c );
    }
  }
};

struct particle
{
  vec3 old_pos;
  vec3 pos;
  vec3 vel;
  vec3 color;
  float size;
  float opacity;
  float gravity_multiplier;
  float life;
};

class particle_manager;

class particle_emitter
{
  int id;
  bool first_update;
  particle_manager* pm;
public:
  int get_id() const
  {
    return id;
  }

  vec3 pos, dir;

  animable_property<vec3> start_pos; //particle is initialized w/ this position relative to pos
  animable_property<vec3> start_velocity; //particle is initialized w/ this velocity relative to dir
  animable_property<vec3> start_color; //particle is initialized w/ this color
  animable_property<float> start_size; //particle is initialized w/ this size
  animable_property<float> start_opacity; //particle is initialized w/ this opacity

  animable_property<float> emit_per_second; //how many particles to emit per second

  animable_property<float> start_life; //the lifetime of a particle

  std::function<vec3( float, const vec3&, const vec3& )> color_over_lifetime;
  std::function<vec3( float, const vec3&, const vec3& )> color_over_speed;
  std::function<float( float, const vec3&, const vec3& )> size_over_lifetime;
  std::function<float( float, const vec3&, const vec3& )> size_over_speed;
  std::function<float( float, const vec3&, const vec3& )> opacity_over_lifetime;
  std::function<float( float, const vec3&, const vec3& )> opacity_over_speed;

  std::vector<std::pair<float, int> > bursts; //when to emit a burst [0...duration], and how many particles to emit

  //TODO
  std::vector<int> birth_subemitter_ids; //makes the sub-emitters emit at this emitter's particles' birth
  std::vector<int> death_subemitter_ids; //makes the sub-emitters emit at this emitter's particles' death

  float life; //how much does the emitter have from it's life (seconds)
  float duration; //the emitter's lifetime (seconds), if looped, one cycle
  bool is_looping; //if true, the particle system won't die
  bool prewarm; //if true, the system will be in a state as if we simulated one cycle

  float gravity_multiplier; //[0...1] how much should gravity affect the particle?

  int max_particles; //maximum number of particles to be present at a time

  bool is_child;

  bool is_additive;

  vector<particle> particles;

  void init( int id, particle_manager* pm )
  {
    this->id = id;
    this->pm = pm;
    first_update = true;
  }

  void emit( float dt, bool sub_birth );

  void emit_bursts( float dt, bool force )
  {
    for( auto& i : bursts )
    {
      if( force || std::abs( i.first - ( duration - life ) ) < dt )
      {
        for( int j = 0; j < i.second; ++j )
        {
          emit( dt, false );
        }
      }
    }
  }

  void update_life( float dt )
  {
    life -= dt;

    if( is_looping && life < 0 )
    {
      life += duration;
    }
  }

  void update_particles( float dt );

  vector<particle>::iterator get_particle_iterator()
  {
    return particles.begin();
  }

  vector<particle>::iterator get_particle_iterator_end()
  {
    return particles.end();
  }

  float elapsed_time;

  void update( float dt )
  {
    if( first_update )
    {
      //init container on first update
      particles.reserve( max_particles );

      //init life
      life = duration;

      elapsed_time = 0;

      first_update = false;

      //prewarm
      if( is_looping && prewarm )
      {
        float time = duration;
        while( time >= 0 )
        {
          float delta = 1 / 60.0f;
          update( delta );
          time -= delta;
        }
      }
    }

    //update particles
    update_particles( dt );

    //bursts
    if( !is_child )
      emit_bursts( dt, false );

    //update life
    update_life( dt );

    //spawn new particles
    if( !is_child )
    {
      elapsed_time += dt;

      float eps = emit_per_second.get( dt, pos, dir );
      float emit_period = 1 / eps;

      if( elapsed_time > emit_period )
      {
        while( elapsed_time > emit_period )
        {
          elapsed_time -= emit_period;
          emit( dt, true );
        }
      }
    }
  }
};

class particle_manager
{
  int id_counter;
  vector<particle_emitter> emitters;
public:

  //returns particle emitter id
  //that uniquely identifies the particle emitter
  //and is guaranteed to always work (pointers and refs may be invalidated after an update())
  int create()
  {
    emitters.push_back( particle_emitter() );
    emitters.back().init( id_counter++, this );
    return emitters.back().get_id();
  }

  //returns a pointer if the emitter is found by the id, if not 0
  //pointer may be invalid after an update
  particle_emitter* get( int id )
  {
    auto it = std::find_if( emitters.begin(), emitters.end(), [&]( const particle_emitter& a )
    {
      return a.get_id() == id;
    } );

    if( it != emitters.end() )
    {
      return &( *it );
    }
    else
    {
      return 0;
    }
  }

  void remove( int id )
  {
    auto it = std::find_if( emitters.begin(), emitters.end(), [&]( const particle_emitter& a )
    {
      return a.get_id() == id;
    } );

    emitters.erase( it );
  }

  void init()
  {
    emitters.reserve( 100 );
    id_counter = 0;
  }

  void update( float dt )
  {
    for( auto& i : emitters )
    {
      i.update( dt );
    }

    for( auto it = emitters.begin(); it != emitters.end(); ++it )
    {
      if( !it->is_looping && it->life < 0 && it->particles.size() == 0 )
      {
        emitters.erase( it );
        it = emitters.begin();

        if( it == emitters.end() )
          break;
      }
    }
  }
};

void particle_emitter::emit( float dt, bool sub_birth )
{
  //emit now
  if( particles.size() < max_particles )
  {
    if( sub_birth )
    {
      for( auto& i : birth_subemitter_ids )
      {
        pm->get( i )->emit_bursts( dt, true );
      }
    }

    particles.push_back( particle() );
    auto& p = particles.back();

    float t = duration - life;
    p.old_pos = start_pos.get( t, pos, dir );
    p.pos = p.old_pos;
    p.vel = start_velocity.get( t, pos, dir );
    p.color = start_color.get( t, pos, dir );
    p.size = start_size.get( t, pos, dir );
    p.opacity = start_opacity.get( t, pos, dir );
    p.gravity_multiplier = gravity_multiplier;
    p.life = start_life.get( t, pos, dir );
  }
}

void particle_emitter::update_particles( float dt )
{
  //TODO new emitter on birth/death

  for( auto& p : particles )
  {
    p.old_pos = p.pos;
    p.vel -= vec3( 0, 10, 0 ) * dt * gravity_multiplier;
    p.pos += p.vel * dt;
    p.life -= dt;
  }

  if( color_over_lifetime )
  {
    for( auto& p : particles )
      p.color = color_over_lifetime( duration - p.life, pos, dir );
  }
  else if( color_over_speed )
  {
    for( auto& p : particles )
      p.color = color_over_speed( length( p.vel ), pos, dir );
  }

  if( size_over_lifetime )
  {
    for( auto& p : particles )
      p.size = size_over_lifetime( duration - p.life, pos, dir );
  }
  else if( size_over_speed )
  {
    for( auto& p : particles )
      p.size = size_over_speed( length( p.vel ), pos, dir );
  }

  if( opacity_over_lifetime )
  {
    for( auto& p : particles )
      p.opacity = opacity_over_lifetime( duration - p.life, pos, dir );
  }
  else if( opacity_over_speed )
  {
    for( auto& p : particles )
      p.opacity = opacity_over_speed( length( p.vel ), pos, dir );
  }

  //remove dead
  for( auto it = particles.begin(); it != particles.end(); ++it )
  {
    if( it->life <= 0 )
    {
      for( auto& i : death_subemitter_ids )
      {
        auto ps = pm->get( i );
        ps->pos = it->pos;
        ps->dir = normalize(it->vel);
        ps->emit_bursts( dt, true );
      }

      particles.erase( it );
      it = particles.begin();

      if( it == particles.end() )
        break;
    }
  }
}
